;(function ($, window){

    $.fn.tapestry = function(options)
    {
        // Make and store a tapestry per container
        if (options === undefined || typeof options === 'object')
        {
            // Setup event handlers for each hyperaction
            $('.hyperaction').on("click", function(){
                var action = $(this).attr("data-action");
                var owner = $(this).attr("for");
                $("#" + owner).data("tapestry").do_action(action);
            });

            return this.each(function(){
                if (!$.data(this, "tapestry"))
                {
                    $.data(this, "tapestry", new Tapestry(this, options));
                }   
            }); 
        }

        // TODO:If the options is a string then expose the plugin's methods
    }

    function Tapestry(element, options)
    {
        this.element = element;
        this.settings = $.extend({}, $.fn.tapestry.settings, options);
        this.init();
    }

    function generate_uuid()
    {
        return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
                var r = Math.random()*16|0, v = c == 'x' ? r : (r&0x3|0x8);
                    return v.toString(16);
        });
    }

    Tapestry.prototype.init = function()
    {
        this.canceler = 0;
        this.cached_images = [];
        this.camera = null;
        this.is_drag = false;
        this.linked_objs = [];
        this.id = generate_uuid();
        this.timeseries_timer = null;
        this.current_timestep = 0;
        this.timerange = [0, 0];
        this.timelog = {};
        
        $(this.element).attr("width", this.settings.width);
        $(this.element).attr("height", this.settings.height);

        $(this.element).css("width", this.settings.width.toString() + "px");
        $(this.element).css("height", this.settings.height.toString() + "px");

        if ($(this.element).attr("data-timerange"))
        {
            var range = $(this.element).attr("data-timerange").split("..");
            this.timerange[0] = parseInt(range[0]);
            this.timerange[1] = parseInt(range[1]);
        }

        if ($(this.element).attr("data-isovalues"))
        {
            this.settings.do_isosurface = true;
            this.settings.isovalues = 
                $(this.element).attr("data-isovalues")
                .split(",").map(function(i){return parseFloat(i)});
        }

        if ($(this.element).attr("data-filters"))
        {
            this.settings.filters = $(this.element).attr("data-filters").split(",");
        }

        // First render
        this.setup_camera();
        this.setup_handlers();
        $(this.element).mousedown();
        $(this.element).mouseup();
    }

    Tapestry.prototype.setup_camera = function(position, up)
    {
        this.camera = new ArcBall();
        this.camera.up = (typeof up !== 'undefined' ? up : $V([0, 1, 0, 1.0]));
        this.camera.position = (typeof position !== 'undefined' ? position : $V([0, 0, this.settings.zoom, 1.0]));

        this.camera.setBounds(this.settings.width, this.settings.height);
        this.camera.zoomScale = this.camera.position.elements[2];
    }

    Tapestry.prototype.getCameraInfo = function()
    {
        var m = $M(this.camera.Transform);
        m = m.inverse();

        var new_camera_position = m.multiply(this.camera.position);
        var new_camera_up = m.multiply(this.camera.up);

        var x = new_camera_position.elements[0];
        var y = new_camera_position.elements[1];
        var z = new_camera_position.elements[2];

        var upx = new_camera_up.elements[0];
        var upy = new_camera_up.elements[1];
        var upz = new_camera_up.elements[2];

        return { position: new_camera_position.elements, up: new_camera_up.elements };
    }

    Tapestry.prototype.render = function(lowquality, remote_call)
    {
        if (typeof remote_call === 'undefined')
        {
            remote_call = false;
        }

        var m = $M(this.camera.Transform);
        m = m.inverse();

        var new_camera_position = m.multiply(this.camera.position);
        var new_camera_up = m.multiply(this.camera.up);

        var precision = 3;
        var x = new_camera_position.elements[0].toFixed(precision);
        var y = new_camera_position.elements[1].toFixed(precision);
        var z = new_camera_position.elements[2].toFixed(precision);

        precision = 3;
        var upx = new_camera_up.elements[0].toFixed(precision);
        var upy = new_camera_up.elements[1].toFixed(precision);
        var upz = new_camera_up.elements[2].toFixed(precision);

        var viewx = -x;
        var viewy = -y;
        var viewz = -z;

        var dataset = $(this.element).attr("data-dataset");
        
        var options = "";
        if ($(this.element).attr("data-colormap"))
        {
            options += "colormap," + $(this.element).attr("data-colormap");
        }

        if (this.settings.n_timesteps > 1)
        {
            options += "timestep," + (this.current_timestep + this.timerange[0]);
        }

        if (this.settings.do_isosurface == true)
        {
            options += "isosurface," 
                + this.settings.isovalues.toString().replace(/,/g, "-");
        }

        // add filters if any
        if (this.settings.filters.length > 0)
        {
            options += "filters,"
                + this.settings.filters.join("-");
        }

        var quality = lowquality;
        if (lowquality == 0)
        {
            quality = this.settings.width;
        }

        var host = "";
        if (this.settings.host !== undefined)
        {
            host = this.settings.host + "/";
        }

        var path = host + "image/" + dataset + "/" + x + "/" + y + "/" + z
            + "/" + upx + "/" + upy + "/" + upz + "/"
            + viewx + "/" + viewy + "/" + viewz + "/"
            + quality.toString() + "/" + options;

        // Let's cache a bunch of the images so that requests
        // don't get cancelled by the browser. 
        // Cancelled requests causes the server to give up/become
        // slow for a specific client probably due to TCP timeouts.
        var temp = new Image();
        temp.src = path;
        this.timelog[temp.src] = [Date.now(), lowquality, false, 0];
        
        temp.onload = $.proxy(function(ev){
            this.timelog[ev.target.src][3] = Date.now();
            this.timelog[ev.target.src][2] = true;
        }, this);

        this.cached_images.push(temp);
        if (this.cached_images.length > this.settings.max_cache_length)
        {
            this.cached_images.splice(0, Math.floor(this.settings.max_cache_length / 2));
        }
        $(this.element).attr("src", path);

        // Don't rotate linked views if this call is
        // from one of them otherwise it'll be an infinite
        // loop
        if (!remote_call)
        {
            for (var i = 0; i < this.linked_objs.length; i++)
            {
                this.linked_objs[i].render(lowquality, true);
            }
        }
    }

    Tapestry.prototype.getInteractionStats = function(host)
    {
        var low_quality_sum = 0;
        var low_quality_n = 0;
        var high_quality_sum=0; 
        var high_quality_n=0;
        for (i in this.timelog)
        {
            if (this.timelog[i][1] && this.timelog[i][2])
            {
                low_quality_n++;
                low_quality_sum += this.timelog[i][3] - this.timelog[i][0];
            }
            else if (!this.timelog[i][1] && this.timelog[i][2])
            {
                high_quality_n++;
                high_quality_sum += this.timelog[i][3] - this.timelog[i][0];
            }
        }
        console.log("Average low quality time of response: ", low_quality_sum / low_quality_n);
        console.log("Average high quality time of response: ", high_quality_sum / high_quality_n);
        console.log("Number of answered requests: ", high_quality_n + low_quality_n);
        console.log("Number of requests sent: ", Object.keys(this.timelog).length);

        var self = this;
        if (typeof host !== 'undefined')
        {
            // Send to log server 
            $.ajax({
                url: host,
                type: 'POST',
                contentType: 'application/json',
                data: JSON.stringify({'load': self.timelog}),
                success: function(){}
            });
        }
        this.timelog = {};
    }

    Tapestry.prototype.rotate = function(mouse_x, mouse_y, lowquality)
    {
        if (this.is_drag)
        {
            this.is_drag = false;
            this.camera.move(mouse_x, mouse_y);
            this.render(lowquality);
            this.is_drag = true;
        }
    }
    
    Tapestry.prototype.unlink_camera = function()
    { 
        this.setup_camera();
        this.render(0);
    }
    
    Tapestry.prototype.link = function(target)
    {
        if (this.linked_objs.indexOf(target) == -1)
        {
            target.camera = this.camera;
            this.settings.camera_link_status = 2;

            /*for (i in this.linked_objs)
            {
                this.linked_objs[i].link(target);
            }*/

            this.linked_objs.push(target);
            // Add ourself to that object too
            target.linked_objs.push(this);

            target.render(0);
        }
    }
    
    // Currently, the target's camera gets reset to the original position
    // after unlinking.
    Tapestry.prototype.unlink = function(target, stop_recursion)
    {
        for (var i = 0; i < this.linked_objs.length; i++)
        {
            if (target.id == this.linked_objs[i].id)
            {
                this.linked_objs.splice(i, 1);
                if (!stop_recursion)
                {
                    target.unlink(this, true);
                    target.unlink_camera();
                }
            }
        } 
    }

    Tapestry.prototype.do_action = function(action)
    {
        var operator_index = action.search(/\+|=|\(|\)/);
        var operation = action.slice(0, operator_index);
        if (operation == 'position')
        {
            var position = action.slice(operator_index + 1);
            position = position.split(",");
            
            var m = $M(this.camera.Transform);
            m = m.inverse();

            var current_position = Vector.create(m.multiply(this.camera.position).elements.slice(0, 3));
            var pos = Vector.create([parseInt(position[0]), parseInt(position[1]), parseInt(position[2])]);
            this.camera.rotateTo(pos);
            this.render(0);
            return this;

            var iteration = 0;
            var self = this;
            animation_timer = setInterval(function(){
                var temp_pos = pos
                    .x(iteration / 100.0)
                    .add(current_position.x(1 - iteration / 100.0));
                self.camera.rotateTo(temp_pos);
                self.render(1);
                if (iteration++ > 100)
                {
                    clearInterval(animation_timer);
                    self.render(0);
                }
            }, 50);
        }
        else if (operation == 'link')
        {
            var targets = action.slice(operator_index + 1);
            targets = targets.replace(/\(|\)| /g, "");
            targets = targets.split(",");
            for (var i = 0; i < targets.length; i++)
            {
                this.link($("#" + targets[i]).data("tapestry"));
            }
        }
        else if (operation == 'unlink')
        {
            var targets = action.slice(operator_index + 1);
            targets = targets.replace(/\(|\)| /g, "");
            targets = targets.split(",");
            for (var i = 0; i < targets.length; i++)
            {
                this.unlink($("#" + targets[i]).data("tapestry"), false);
            }
        }
        else if (operation == 'play')
        {
            self = this;
            this.timeseries_timer = setInterval(function(){
                self.current_timestep = (self.current_timestep + 1) % (self.timerange[1] - self.timerange[0]);
                self.render(self.is_drag + 0);
            }, this.settings.animation_interval);
        }
        else if (operation == 'stop')
        {
            clearInterval(this.timeseries_timer);
        }
        else if (operation == 'switch_config')
        {
            var targets = action.slice(operator_index + 1);
            targets = targets.replace(/\(|\)| /g, "");
            targets = targets.split(",");
            $(this.element).attr("data-dataset", targets[0]);
            this.render(0);
        }

    }

    Tapestry.prototype.setup_handlers = function()
    {
        var self = this;
        $(this.element).on("mousedown", function(){
            self.is_drag = true;

            self.camera.LastRot = self.camera.ThisRot;
            self.camera.click(event.clientX - self.element.getBoundingClientRect().left, event.clientY - self.element.getBoundingClientRect().top);

            return false;
        });

        $(this.element).on("mousemove", function(){
            self.canceler = (self.canceler + 1) % 1000;
            if (self.canceler % 5 == 0)
            {
                var mouse_x = event.clientX - self.element.getBoundingClientRect().left;
                var mouse_y = event.clientY - self.element.getBoundingClientRect().top;
                self.rotate(mouse_x, mouse_y, 1); // Render low quality version
            }
        });

        $(this.element).on("mouseup", function(event){
            var mouse_x = event.clientX - self.element.getBoundingClientRect().left;
            var mouse_y = event.clientY - self.element.getBoundingClientRect().top;

            self.rotate(mouse_x, mouse_y, 0); // Render high quality version
            self.is_drag = false;
            return false;
        });
        
        $(this.element).on("dragstart", function(event){
            event.preventDefault();
        });

        $(this.element).on("mousewheel", function(event){
            if (self.settings.enableZoom == false)
                return false;
            self.camera.zoomScale -= event.originalEvent.wheelDeltaY * 0.1;
            self.camera.position.elements[2] = self.camera.zoomScale;
            self.render(1);

            clearTimeout($.data(self, 'timer'));
            $.data(self, 'timer', setTimeout(function() {
                self.render(0);
            }, 1000));
            return false;
        });

        // MOA::BROKEN
        /*
         * Setup camera linking using a double click event
         */
        /*$(this.element).on("dblclick", function(){
            if (self.settings.camera_link_status == 0)
            {
                // Look for others
                var found = false;
                $(".hyperimage").each(function(){
                    if ($(this).data("tapestry").settings.camera_link_status == 1)
                    {
                        self.camera = $(this).data("tapestry").camera;
                        self.settings.camera_link_status = 2;
                        self.linked_objs.push($(this).data("tapestry"));
                        // Add ourself to that object too
                        $(this).data("tapestry").linked_objs.push(self);
                    }
                });

                // If we didn't find a hyperimage ready to be linked then make this
                // one ready to be linked
                if (found == false)
                {
                    self.settings.camera_link_status = 1;
                }
            }
            else
            {
                // Reset the camera
                var current_camera_position = self.camera.position;
                var current_camera_up = self.camera.up;
                self.setup_camera(current_camera_position, current_camera_up);
                
                // TODO: Clean up all linkages of this object
                
            }
        });*/

        /* 
         * Touch event handlers
         */
        $(this.element).on("touchstart", function(event){
            self.is_drag = true;
            console.log("touchstart");

            //update the base rotation so model doesn't jerk around upon new clicks
            self.camera.LastRot = self.camera.ThisRot;

            //tell the camera where the touch event happened
            self.camera.click(event.originalEvent.touches[0].clientX - 
                    self.element.getBoundingClientRect().left, event.originalEvent.touches[0].clientY - 
                    self.element.getBoundingClientRect().top);

            return false;
        });

        //handle touchEnd
        $(this.element).on("touchend", function(event){
            console.log("touchend");
            self.is_drag = false;

            self.render(0);
            return false;
        });

        //handle touch movement
        $(this.element).on("touchmove", function(event){
            console.log("touchmove");
            if (self.is_drag == true)
            {
                mouse_x = event.originalEvent.touches[0].clientX - self.element.getBoundingClientRect().left;
                mouse_y = event.originalEvent.touches[0].clientY - self.element.getBoundingClientRect().top;

                self.rotate(mouse_x, mouse_y, 1); // Render low quality version
            }
            return false;
        });

    }

    /*
     * Default settings for a tapestry object
     */
    $.fn.tapestry.settings = {
        host: "",
        width: 512,
        height: 512,
        zoom: 512,
        max_cache_length: 512,
        enable_zoom: true,
        enable_rotation: true,
        animation_interval: 100,
        n_timesteps: 1,
        do_isosurface: false,
        isovalues: [0], 
        filters: [],
        camera_link_status: 0 // 0: Not linked, 1: Waiting to be linked, 2: Linked
    };

}(jQuery, window));

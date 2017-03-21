;(function ($, window){

    $.fn.tapestry = function(options)
    {
        // Make and store a tapestry per container
        if (options === undefined || typeof options === 'object')
        {
            return this.each(function(){
                if (!$.data(this, "tapestry"))
                {
                    $.data(this, "tapestry", new Tapestry(this, options));
                }   
            }); 
        }

        // If the options is a string then expose the plugin's methods
    }

    function Tapestry(element, options)
    {
        this.element = element;
        this.settings = $.extend({}, $.fn.tapestry.settings, options);
        this.init();
    }

    Tapestry.prototype.init = function()
    {
        this.canceler = 0;
        this.cached_images = [];
        this.camera = null;
        this.is_drag = false;
        
        $(this.element).attr("width", this.settings.width);
        $(this.element).attr("height", this.settings.height);

        $(this.element).css("width", this.settings.width.toString() + "px");
        $(this.element).css("height", this.settings.height.toString() + "px");

        // First render
        this.setup_camera();
        this.setup_handlers();
        $(this.element).mousedown();
        $(this.element).mouseup();
    }

    $.fn.tapestry.settings = {
        width: 512,
        height: 512,
        max_cache_length: 512,
        enable_zoom: true,
        enable_rotation: true
    };

    Tapestry.prototype.setup_camera = function(position, up)
    {
        this.camera = new ArcBall();
        this.camera.up = (typeof up !== 'undefined' ? up : $V([0, 1, 0, 1.0]));
        this.camera.position = (typeof position !== 'undefined' ? position : $V([0, 0, 512, 1.0]));

        this.camera.setBounds(this.settings.width, this.settings.height);
        this.camera.zoomScale = this.camera.position.elements[2];
    }

    Tapestry.prototype.render = function(lowquality)
    {
        var m = $M(this.camera.Transform);
        m = m.inverse();

        var new_camera_position = m.multiply(this.camera.position);
        var new_camera_up = m.multiply(this.camera.up);
        
        var x = new_camera_position.elements[0].toFixed(3);
        var y = new_camera_position.elements[1].toFixed(3);
        var z = new_camera_position.elements[2].toFixed(3);

        var upx = new_camera_up.elements[0].toFixed(3);
        var upy = new_camera_up.elements[1].toFixed(3);
        var upz = new_camera_up.elements[2].toFixed(3);
        
        var path = "/image/magnetic/" + x + "/" + y + "/" + z
            + "/" + upx + "/" + upy + "/" + upz + "/"
            + lowquality.toString();

        // Let's cache a bunch of the images so that requests
        // don't get cancelled by the browser. 
        // Cancelled requests causes the server to give up/become
        // slow for a specific client probably due to TCP timeouts.
        var temp = new Image();
        temp.src = path;
        this.cached_images.push(temp);
        if (this.cached_images.length > this.settings.max_cache_length)
        {
            this.cached_images.splice(0, Math.floor(this.settings.max_cache_length / 2));
        }
        $(this.element).attr("src", path);
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

    Tapestry.prototype.setup_handlers = function()
    {
        var self = this;
        $(this.element).on("mousedown", function(){
            self.is_drag = true;

            self.camera.LastRot = self.camera.ThisRot;
            self.camera.click(event.clientX - $(self.element).offset().left, event.clientY - $(self.element).offset().top);

            return false;
        });

        $(this.element).on("mousemove", function(){
            self.canceler = (self.canceler + 1) % 1000;
            if (self.canceler % 5 == 0)
            {
                var mouse_x = event.clientX - $(self.element).offset().left;
                var mouse_y = event.clientY - $(self.element).offset().top;
                self.rotate(mouse_x, mouse_y, 1); // Render low quality version
            }
        });

        $(this.element).on("mouseup", function(event){
            var mouse_x = event.clientX - $(self.element).offset().left;
            var mouse_y = event.clientY - $(self.element).offset().top;

            self.rotate(mouse_x, mouse_y, 0); // Render high quality version
            self.is_drag = false;
            return false;
        });
        
        $(this.element).on("dragstart", function(event){
            event.preventDefault();
        });

        $(this.element).on("mousewheel", function(event){
            self.camera.zoomScale -= event.originalEvent.wheelDeltaY * 0.1;
            self.camera.position.elements[2] = self.camera.zoomScale;
            self.render(1);

            clearTimeout($.data(self, 'timer'));
            $.data(self, 'timer', setTimeout(function() {
                self.render(0);
            }, 1000));
            return false;
        });

        $(this.element).on("touchstart", function(event){
            self.is_drag = true;

            //update the base rotation so model doesn't jerk around upon new clicks
            self.camera.LastRot = this.camera.ThisRot;

            //tell the camera where the touch event happened
            self.camera.click(event.originalEvent.touches[0].clientX - 
                    $(self.element).offset().left, event.originalEvent.touches[0].clientY - 
                    $(self.element).offset().top);

            return false;
        });

        //handle touchEnd
        $(this.element).on("touchend", function(event){
            self.is_drag = false;

            self.render(0);
            return false;
        });

        //handle touch movement
        $(this.element).on("touchmove", function(event){
            if (self.is_drag == true)
            {
                mouse_x = event.originalEvent.touches[0].clientX - $(self.element).offset().left;
                mouse_y = event.originalEvent.touches[0].clientY - $(self.element).offset().top;

                self.rotate(mouse_x, mouse_y, 1); // Render low quality version
            }
            return false;
        });
    }

}(jQuery, window));

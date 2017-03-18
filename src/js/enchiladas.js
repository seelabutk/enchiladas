$(document).ready(function(){
    width = 512;
    height = 512;

    canceler = 0;
    cached_images = [];
    MAX_CACHE_LENGTH = 512;

    camera = new ArcBall();
    camera.setBounds(width, height);
    camera.up = $V([0, 1, 0, 1.0]);
    camera.position = $V([0, 0, 512, 1.0]);
    camera.zoomScale = camera.position.elements[2];

    var is_drag = false;

    var img_tag = $("#img");
    img_tag.attr("width", width);
    img_tag.attr("height", height);

    img_tag.css("width", width.toString() + "px");
    img_tag.css("height", height.toString() + "px");

    img_tag_x = img_tag.offset().left;
    img_tag_y = img_tag.offset().top;

    $(window).resize(function()
    {            
        img_tag_x = img_tag.offset().left;
        img_tag_y = img_tag.offset().top;
    });
    
  	function getUrlParameter(sParam) {
        var sPageURL = decodeURIComponent(window.location.search.substring(1)),
            sURLVariables = sPageURL.split('&'),
            sParameterName,
            i;

        for (i = 0; i < sURLVariables.length; i++) {
            sParameterName = sURLVariables[i].split('=');

            if (sParameterName[0] === sParam) {
                return sParameterName[1] === undefined ? true : sParameterName[1];
            }
        }
    }; 

    function render(lowquality)
    {
        var m = $M(camera.Transform);
        m = m.inverse();

        var new_camera_position = m.multiply(camera.position);
        var new_camera_up = m.multiply(camera.up);
        
        var x = new_camera_position.elements[0].toFixed(3);
        var y = new_camera_position.elements[1].toFixed(3);
        var z = new_camera_position.elements[2].toFixed(3);

        var upx = new_camera_up.elements[0].toFixed(3);
        var upy = new_camera_up.elements[1].toFixed(3);
        var upz = new_camera_up.elements[2].toFixed(3);
        
        var path = "/image/" + x + "/" + y + "/" + z
            + "/" + upx + "/" + upy + "/" + upz + "/"
            + lowquality.toString();

        // Let's cache a bunch of the images so that requests
        // don't get cancelled by the browser. 
        // Cancelled requests causes the server to give up/become
        // slow for a specific client probably due to TCP timeouts.
        var temp = new Image();
        temp.src = path;
        cached_images.push(temp);
        if (cached_images.length > MAX_CACHE_LENGTH)
        {
            cached_images.splice(0, Math.floor(MAX_CACHE_LENGTH / 2));
        }
        img_tag.attr("src", path);
    }

    function rotate(mouse_x, mouse_y, lowquality)
    {
        if (is_drag)
        {
            is_drag = false;
            camera.move(mouse_x, mouse_y);
            render(lowquality);
            is_drag = true;
        }
    }

    img_tag.on("mousedown", function(){
        is_drag = true;

        camera.LastRot = camera.ThisRot;
        camera.click(event.clientX - img_tag_x, event.clientY - img_tag_y);

        return false;
    });

    img_tag.on("mousemove", function(){
        canceler = (canceler + 1) % 1000;
        if (canceler % 5 == 0)
        {
            var mouse_x = event.clientX - img_tag_x;
            var mouse_y = event.clientY - img_tag_y;
            rotate(mouse_x, mouse_y, 1); // Render low quality version
        }
    });

    img_tag.on("mouseup", function(event){
        var mouse_x = event.clientX - img_tag_x;
        var mouse_y = event.clientY - img_tag_y;

        rotate(mouse_x, mouse_y, 0); // Render high quality version
        is_drag = false;
        return false;
    });
    
    img_tag.on("dragstart", function(event){
        event.preventDefault();
    });

    img_tag.on("mousewheel", function(event){
        camera.zoomScale -= event.originalEvent.wheelDeltaY * 0.1;
        camera.position.elements[2] = camera.zoomScale;
        render(1);

        clearTimeout($.data(this, 'timer'));
        $.data(this, 'timer', setTimeout(function() {
            render(0);
        }, 1000));
        return false;
    });

    img_tag.on("touchstart", function(event){
        is_drag = true;

        //update the base rotation so model doesn't jerk around upon new clicks
        camera.LastRot = camera.ThisRot;

        //tell the camera where the touch event happened
        camera.click(event.originalEvent.touches[0].clientX - img_tag_x, event.originalEvent.touches[0].clientY - img_tag_y);

        return false;
    });

    //handle touchEnd
    img_tag.on("touchend", function(event){
        is_drag = false;

        render(0);
        return false;
    });

    //handle touch movement
    img_tag.on("touchmove", function(event){
        if (is_drag == true)
        {
            mouse_x = event.originalEvent.touches[0].clientX - img_tag_x;
            mouse_y = event.originalEvent.touches[0].clientY - img_tag_y;

            rotate(mouse_x, mouse_y, 1); // Render low quality version
        }
        return false;
    });

    // First render
    img_tag.mousedown();
    img_tag.mouseup();

	// Set up testing if needed
	if (getUrlParameter("test"))
    {
        tester = new Tester();
        tester.test();
    }
});         

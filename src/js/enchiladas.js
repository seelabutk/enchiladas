$(document).ready(function(){
    width = 512;
    height = 512;

    camera = new ArcBall();
    camera.setBounds(width, height);
    camera.up = $V([0, 1, 0, 1.0]);
    camera.position = $V([0, 0, 1024, 1.0]);
    camera.zoomScale = 1.0;

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
    
    function render(lowquality)
    {
        var m = $M(camera.Transform);
        m = m.inverse();

        var new_camera_position = m.multiply(camera.position);
        var new_camera_up = m.multiply(camera.up);
        
        var x = new_camera_position.elements[0].toFixed().toString();
        var y = new_camera_position.elements[1].toFixed().toString();
        var z = new_camera_position.elements[2].toFixed().toString();

        var upx = new_camera_up.elements[0].toString();
        var upy = new_camera_up.elements[1].toString();
        var upz = new_camera_up.elements[2].toString();
        
        img_tag.attr("src", "/image/" + x + "/" + y + "/" + z 
                + "/" + upx + "/" + upy + "/" + upz + "/" 
                + lowquality.toString());
    }

    function rotate(lowquality)
    {
        if (is_drag)
        {
            is_drag = false;
            var mouse_x = event.clientX - img_tag_x;
            var mouse_y = event.clientY - img_tag_y;
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
        rotate(1); // Render low quality version
    });

    img_tag.on("mouseup", function(event){
        rotate(0); // Render high quality version
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
        }, 250));
        return false;
    });
});         

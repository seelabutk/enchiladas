$(document).ready(function(){
    width = 250;
    bias = 1.0;
    camera_position = [0, 0, 1024, 1.0];
    camera_up = [0, 1, 0, 1.0];
    camera = new ArcBall();
    camera.setBounds(width, width);
    camera.lookAt = [0, 0, 0];

    var is_drag = false;

    var img_tag = $("#img");
    img_tag.on("mousedown", function(){
        is_drag = true;

        camera.LastRot = camera.ThisRot;
        camera.click(event.clientX / bias, event.clientY / bias);

        return false;
    });

    function rotate(rotate)
    {
        //https://forum.libcinder.org/topic/apply-and-arcball-rotation-to-a-camera
        if (is_drag)
        {
            is_drag = false;
            var mouse_x = event.clientX / bias;
            var mouse_y = event.clientY / bias;
            camera.move(mouse_x, mouse_y);
            

            v_position = $V(camera_position);

            v_up = $V(camera_up);
            m = $M(camera.Transform);
            m = m.inverse();
            camera_position = m.multiply(v_position);
            
            camera_up = m.multiply(v_up);
            
            var x = camera_position.elements[0].toFixed().toString();
            var y = camera_position.elements[1].toFixed().toString();
            var z = camera_position.elements[2].toFixed().toString();

            var upx = camera_up.elements[0].toString();
            var upy = camera_up.elements[1].toString();
            var upz = camera_up.elements[2].toString();
            
            camera_position = [0, 0, 1024, 1.0]; //camera_position.elements;
            camera_up = [0, 1, 0, 1.0]; //camera_up.elements;
            console.log(event.clientX, event.clientY, x, y, z);
            img_tag.attr("src", "/image/" + x + 
                    "/" + y + "/" + z + "/" + upx + "/" + upy + "/" + upz + "/" + rotate.toString());
            is_drag = true;
        }
    }

    img_tag.on("mousemove", function(){
        rotate(1);
    });


    img_tag.on("mouseup", function(event){
        rotate(0);
        is_drag = false;
        return false;
    });
    
    img_tag.on("dragstart", function(event){
        event.preventDefault();
    });
});         

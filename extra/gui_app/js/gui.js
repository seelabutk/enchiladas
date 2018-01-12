sortable = null;
$(document).ready(function(){
    var timesteps = 60;
    $(".hyperimage").tapestry({n_timesteps: timesteps});
    var simpleList = document.getElementById("simpleList");
    sortable = Sortable.create(simpleList, {handle: ".handle"});

    $("#btn-add-frame").on("click", function(){
        var hyperimage = $("<img>")
            .addClass("hyperimage")
            .addClass("keyframe")
            .attr("id", "supernova")
            .attr("data-dataset", "supernova")
            .attr("src", "/image")
            .attr("width", 100)
            .attr("height", 100);

        var handle = $("<span>")
            .addClass("handle")
            .text("::");
        
        var new_frame = $("<li>")
            .addClass("list-group-item")
            .append(handle);

        new_frame.append(hyperimage);
        sortable.el.appendChild(new_frame.get(0));
        var hyperimage_obj = $(hyperimage).tapestry({
            width: 100, 
            height: 100,    
            n_timesteps: timesteps
        });
        var main_hyperimage_camera_info = 
            $(".main-hyperimage").data("tapestry").getCameraInfo();
        $(hyperimage_obj).data("tapestry").camera.ThisRot = 
            $(".main-hyperimage").data("tapestry").camera.ThisRot;
        $(hyperimage_obj).data("tapestry").camera.zoomScale = 
            $(".main-hyperimage").data("tapestry").camera.zoomScale;
        $(hyperimage_obj).mousedown();
        $(hyperimage_obj).mouseup();
    });

    $("#btn-animate").on("click", function(){
        var keyframes = [];
        $(".keyframe").each(function(){
            var temp_frame = [];
            temp_frame.push($(this).data("tapestry").camera.ThisRot);
            temp_frame.push($(this).data("tapestry").camera.zoomScale);
            if ($(this).data("tapestry").settings.n_timesteps > 1)
            {
                temp_frame.push($(this).data("tapestry").current_timestep);
            }
            keyframes.push(temp_frame);
        }); 
        $(".main-hyperimage").data("tapestry").keyframes = keyframes;
        $(".main-hyperimage").data("tapestry").animate();
    });
});

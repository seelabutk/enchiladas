$(document).ready(function(){
    $(".hyperimage").tapestry({
        n_tiles: 16,
        width: 1024,
        height: 1024
    });

    // Listen to slider events and change the 
    // isosurface threshold accordingly
    $(".threshold-slider").on("input", function(){
        $(".hyperimage").eq(1).data("tapestry")
            .settings.isovalues=[parseInt($(this).val())];
        $(".hyperimage").eq(1).data("tapestry").render(0);
    });
});

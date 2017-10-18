$(document).ready(function(){
    $(".hyperimage").tapestry({});

    // Listen to slider events and change the 
    // isosurface threshold accordingly
    $(".threshold-slider").on("input", function(){
        $(".hyperimage").eq(1).data("tapestry")
            .settings.isovalues=[parseInt($(this).val())];
        $(".hyperimage").eq(1).data("tapestry").render(0);
    });
});

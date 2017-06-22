$(document).ready(function(){
    if (getUrlParameter("bgcolor"))
    {
        $("body").css({'background-color': getUrlParameter("bgcolor")});
    }

    $(".hyperimage").tapestry({});

    var strip_width = 1;
    var degrees = 2;
    var output_height = 180 / degrees;
    var output_width = 360 / degrees; // canvas width

    var render_width = 64;
    var render_height = 64;

    var object_radius = 220;
    var canvas = $("<canvas>").appendTo("body");
    $("canvas").attr("width", output_width);
    $("canvas").attr("height", output_height);
    context = $("canvas").get(0).getContext("2d");
    ahi = 0;
    ahj = 0;
    $(".hyperimage").eq(0).data("tapestry").camera.rotateTo($V([0, 0, object_radius]));
    $(".hyperimage").eq(0).data("tapestry").camera.rotateByAngle(-90, 'x', [0, 0, object_radius]);
    setInterval(function(){
        $(".hyperimage").eq(0).data("tapestry").camera.rotateByAngle(degrees, 'x', [0, 0, object_radius]);
        $(".hyperimage").eq(0).data("tapestry").render(64);
        var img = $(".hyperimage").eq(0).data("tapestry").element;
        var len = $(".hyperimage").eq(0).data("tapestry").cached_images.length;
        $(".hyperimage").eq(0).data("tapestry").cached_images[len-1].onload = function(){
            context.drawImage(this, render_height / 2, render_width / 2, 
                    1, 1, ahi, ahj, 1, 1);
        }
        if (++ahj == output_height)
        {
            ahj = 0;
            ahi++;
            $(".hyperimage").eq(0).data("tapestry").camera.rotateByAngle(180, 'x', [0, 0, object_radius]);
            $(".hyperimage").eq(0).data("tapestry").camera.rotateByAngle(degrees, 'y', [0, 0, object_radius]);
        }
    }, 100);
});

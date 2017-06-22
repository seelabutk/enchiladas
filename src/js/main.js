$(document).ready(function(){
    if (getUrlParameter("bgcolor"))
    {
        $("body").css({'background-color': getUrlParameter("bgcolor")});
    }

    $(".hyperimage").tapestry({});
});

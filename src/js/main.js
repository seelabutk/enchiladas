$(document).ready(function(){
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

    $(".hyperimage").tapestry({"host": "http://accona.eecs.utk.edu:8010/"});

	// Set up testing if needed
	if (getUrlParameter("test"))
    {
        tester = new Tester();
        tester.test();
    }
});

Tester = function(configs)
{
    horde = null;
    counter = 0;
    clickTypes = ['mousedown', 'mousemove', 'mouseup'];
    this.element = $(configs.element);
    prevX = this.element.outerWidth() / 2.0;
    prevY = this.element.outerHeight() / 2.0;
    randy = function(){};
    self = this;
    randy.pick = function()
    {
        var finish_move = Math.random();
        var mouse_up_prob = 0.15;
        if (counter == 1 && finish_move < mouse_up_prob)
        {
            counter = 2;
            prevX = self.element.outerWidth() / 2.0;
            prevY = self.element.outerHeight() / 2.0;
            return 'mouseup';
        }
        else if (counter == 1 && finish_move >= mouse_up_prob)
        {
            counter = 1;
            return 'mousemove';
        }
        if (counter == 0)
        {
            counter = 1;
            return 'mousemove';
        }
        if (counter == 2)
        {
            counter = 0;
            return 'mousedown';
        }
    }


    this.test = function()
    {
        var counter = 0;
        self = this;
        var custom_toucher = gremlins.species.clicker()
            .clickTypes(['mousedown', 'mousemove', 'mouseup'])
            .canClick(function(element){
                if ($(element).hasClass("tapestry-test"))
                {
                    return true;
                }
                return false;
            })
            .positionSelector(function(){
                var offset = self.element.offset();
                newPos = [(prevX + Math.random() * 20) % self.element.outerWidth() - 10.0, 
                    (prevY + Math.random() * 20) % self.element.outerHeight() - 10.0];
                prevX = newPos[0];
                prevY = newPos[1];

                return [newPos[0] + offset.left, newPos[1] + offset.top];
                
            })
            .randomizer(randy);
        horde = gremlins.createHorde()
            .gremlin(custom_toucher);

        horde.strategy(gremlins.strategies.distribution()
                .delay(20)
                .distribution([0.33, 0.33, 0.33])
        );
        horde.unleash({nb: 10000});
    }
}
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

	// Set up testing if needed
	if (getUrlParameter("test"))
    {
        setTimeout(function(){
            $(".tapestry-test").each(function(){
                tester = new Tester({element: this});
                tester.test();
            });
        }, 1000);
    }
});

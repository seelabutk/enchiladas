Tester = function(configs)
{
    horde = null;
    state_machine_counter = 0;
    clickTypes = ['mousedown', 'mousemove', 'mouseup'];
    this.element = $(configs.element);
    prevX = this.element.outerWidth() / 2.0;
    prevY = this.element.outerHeight() / 2.0;
    randy = new Chance();
    self = this;
    mouse_up_prob = 1.0;
    randy.pick = function()
    {
        var finish_move = Math.random();
        if (state_machine_counter == 1 && finish_move < mouse_up_prob)
        {
            state_machine_counter = 2;
            prevX = self.element.outerWidth() / 2.0;
            prevY = self.element.outerHeight() / 2.0;
            mouse_up_prob = 0.01;
            return 'mouseup';
        }
        else if (state_machine_counter == 1 && finish_move >= mouse_up_prob)
        {
            state_machine_counter = 1;
            return 'mousemove';
        }
        if (state_machine_counter == 0)
        {
            state_machine_counter = 1;
            return 'mousemove';
        }
        if (state_machine_counter == 2)
        {
            state_machine_counter = 0;
            return 'mousedown';
        }
    }

    Tester.prototype.test = function(n_gremlins)
    {
        var state_machine_counter = 0;
        self = this;

        var custom_toucher = gremlins.species.clicker()
            .clickTypes(['mousedown', 'mousemove', 'mouseup'])
            .canClick(function(element){
                if ($(element).hasClass("hyperimage"))
                {
                    return true;
                }
                console.log("Not in bounds of the object.");
                return false;
            })
            .positionSelector(function(){
                var offset = self.element.offset();
                newPos = [prevX + ((Math.random() * 8) % self.element.outerWidth() - 4.0), 
                    prevY + ((Math.random() * 8) % self.element.outerHeight() - 4.0)];
                console.log(newPos);
                prevX = newPos[0];
                prevY = newPos[1];

                return [newPos[0] + offset.left, newPos[1] + offset.top];
            })
            .randomizer(randy);

        horde = gremlins.createHorde()
            .gremlin(custom_toucher);

        horde.strategy(gremlins.strategies.distribution()
            .delay(8)
            .distribution([0.33, 0.33, 0.33])
        );
        horde.after(function(){
            $(self.element).data("tapestry").getInteractionStats("http://accona.eecs.utk.edu:8080/");
        });

        horde.unleash({nb: n_gremlins});
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
            var random = Math.floor(Math.random() * 5);
            /*$(".tapestry").eq(random).each(function(){
                tester = new Tester({element: this});
                tester.test();
            });*/
            tester = new Tester({element: $(".hyperimage").get(random)});
            tester.test(getUrlParameter("test"));
        }, 2000);
    }
});

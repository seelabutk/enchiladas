Tester = function(configs)
{
    horde = null;
    counter = 0;
    clickTypes = ['mousedown', 'mousemove', 'mouseup'];
    var $el = $("#img");
    prevX = $el.outerWidth() / 2.0;
    prevY = $el.outerHeight() / 2.0;
    randy = function(){};
    randy.pick = function()
    {
        var finish_move = Math.random();
        var mouse_up_prob = 0.15;
        if (counter == 1 && finish_move < mouse_up_prob)
        {
            counter = 2;
            prevX = $el.outerWidth() / 2.0;
            prevY = $el.outerHeight() / 2.0;
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
        var custom_toucher = gremlins.species.clicker()
            .clickTypes(['mousedown', 'mousemove', 'mouseup'])
            .canClick(function(element){
                if (element.getAttribute("id") == "img")
                {
                    return true;
                }
                return false;
            })
            .positionSelector(function(){
                var $el = $("#img");
                var offset = $el.offset();
                newPos = [(prevX + Math.random() * 20) % $el.outerWidth() - 10.0, 
                    (prevY + Math.random() * 20) % $el.outerHeight() - 10.0];
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

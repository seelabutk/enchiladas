from tapestry_plugin import TapestryPlugin
from PIL import Image
import io
import StringIO

def bw(data):
    image = Image.open(io.BytesIO(data))
    image = image.convert('L')
    print image

    output = StringIO.StringIO()
    format = 'PNG' # or 'JPEG' or whatever you want
    image.save(output, format)
    contents = output.getvalue()
    output.close()
    return contents

plugin = TapestryPlugin('bw', '0.0.0.0', 8050)
plugin.register_function(bw)
plugin.deploy()

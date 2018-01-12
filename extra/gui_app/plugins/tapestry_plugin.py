#!/usr/bin/env python

from flask import Flask, Response, request
from PIL import Image
import io
import StringIO
import sys

class EndpointAction(object):
    def __init__(self, action):
        self.action = action

    def __call__(self, *args):
        result = self.action(request.data)
        return result, 200

class TapestryHttpPlugin(object):
    def __init__(self, name, host, port):
        self.host = host
        self.port = port
        self.app = Flask(name)

    def register_function(self, func):
        self.app.add_url_rule('/' + func.__name__, func.__name__, EndpointAction(func), methods=['POST'])

    def deploy(self):
        self.app.run(host=self.host, port=self.port)


class Tools():
    @staticmethod
    def read_image():
        """
        Reads a Tapestry image from stdin, converts it to a PIL image and returns it
        """
        data = sys.stdin.read()
        image = Image.open(io.BytesIO(data))
        return image

    @staticmethod
    def write_image(pil_image):
        """
        Takes a PIL image and sends it to Tapestry's pipeline to be served
        """

        output = StringIO.StringIO()
        format = 'PNG' # or 'JPEG' or whatever you want
        pil_image.save(output, format)
        contents = output.getvalue()
        output.close()
        sys.stdout.write(contents)

#!/usr/bin/env python
"""Flapgen path extension (modify path)"""

import math
import inkex
import inkex.command
import os
import copy
from lxml import etree
from subprocess import Popen, PIPE

class FlapSplit(inkex.EffectExtension):
    def add_arguments(self, pars):
        pars.add_argument("-e", "--export", type=bool, default=True)
        pars.add_argument("-p", "--output_path", type=str)
        pars.add_argument("-n", "--file_name", type=str, default='Split_flap_Silkscreen')

    def effect(self):
        if not os.path.isdir(self.options.output_path):
            inkex.utils.debug("invalid export path")
            return
        proto_flap = self.svg.xpath("//svg:path[@flap='True']")[0]
        mid_cutout = float(proto_flap.get('mid_cutout'))

        flap_char_ids = ""
        number_of_flaps = len(self.svg.xpath("//svg:g[@id='layer_gen_characterMap']/*"))
        for i in range(number_of_flaps):
            flap_char_ids+= "flap_char_{},".format(i+1)

        self.document = inkex.load_svg(inkex.command.inkscape_command(
            self.svg,
            select=flap_char_ids,
            verbs=["ObjectToPath","SelectionUnGroup"]))  
        self.svg = self.document.getroot()
        selected_ids = self.options.ids

        i=1
        for flap_group in self.svg.xpath("//svg:g[@id='layer_gen_characterMap']/*"):
            if(len(flap_group) == 3):
                flap_group[-1].set_id("top_char_{}".format(i))
                # copy char
                char_copy = copy.deepcopy(flap_group[-1])
                char_copy.set_id("bot_char_{}".format(i))
                flap_group.append(char_copy)
            else:
                if(len(flap_group) >= 1):
                    flap_group.remove(flap_group[0])
                if(len(flap_group) >= 2):
                    flap_group.remove(flap_group[1])
            i+=1

        # intersect the characters
        for i in range(number_of_flaps):
            self.document = inkex.load_svg(inkex.command.inkscape_command(
                self.svg,
                select="top_flap_{},top_char_{}".format(i+1,i+1),
                verbs=["SelectionIntersect"]))  
            self.svg = self.document.getroot()
            self.document = inkex.load_svg(inkex.command.inkscape_command(
                self.svg,
                select="bot_flap_{},bot_char_{}".format(i+1,i+1),
                verbs=["SelectionIntersect"]))  
            self.svg = self.document.getroot()

        # set transforms for cut chars
        for i in range(number_of_flaps): 
            flap_group_top = self.svg.getElementById("top_flap_{}".format(i+1))
            flap_group_bot = self.svg.getElementById("bot_flap_{}".format(i+1))   
            prev_group = self.svg.getElementById("flap_{}".format(i) if i+1 > 1 else "flap_{}".format(number_of_flaps))   
            if(flap_group_top is not None): 
                flap_group_top.style['fill'] = "#ff5599"
                flap_group_top.set('layer','top')
            if(flap_group_bot is not None): 
                flap_group_bot.style['fill'] = "#5f5fd3"
                flap_group_bot.set('layer','bot')
                flip = inkex.Transform("scale(1, -1)")
                old_transform = inkex.Transform (flap_group_bot.transform)
                new_transform = old_transform*flip
                translate = inkex.Transform("translate({},{})".format(0,(proto_flap.bounding_box().height*2 + mid_cutout)/new_transform.d))
                flap_group_bot.set('transform', new_transform*translate)
                prev_group.append(flap_group_bot)

        # set style for proto_flap
        proto_flap = self.svg.xpath("//svg:path[@flap='True']")[0]
        proto_flap.style['fill'] = 'none'
        proto_flap.style['stroke'] = '#ffff00'
        proto_flap.style['stroke-width'] = '0.025'

        for i in range(number_of_flaps): 
            layer_bPlace = self.svg.getElementById("layer_bPlace")
            if(layer_bPlace is None):  
                layer_bPlace_atr={}
                layer_bPlace_atr['id'] = 'layer_bPlace'
                layer_bPlace_atr["{http://www.inkscape.org/namespaces/inkscape}groupmode"] = "layer"
                layer_bPlace_atr["{http://www.inkscape.org/namespaces/inkscape}label"] = "bPlace"
                layer_bPlace = etree.SubElement(self.svg, inkex.addNS('g','svg'), layer_bPlace_atr )
            layer_tPlace = self.svg.getElementById("layer_tPlace")
            if(layer_tPlace is None):  
                layer_tPlace_atr={}
                layer_tPlace_atr['id'] = 'layer_tPlace'
                layer_tPlace_atr["{http://www.inkscape.org/namespaces/inkscape}groupmode"] = "layer"
                layer_tPlace_atr["{http://www.inkscape.org/namespaces/inkscape}label"] = "tPlace"
                layer_tPlace = etree.SubElement(self.svg, inkex.addNS('g','svg'), layer_tPlace_atr )

            # clear layers
            for top in self.svg.xpath("//svg:g[@id='layer_tPlace']/*"):
                layer_tPlace.remove(top)
            for bot in self.svg.xpath("//svg:g[@id='layer_bPlace']/*"):
                layer_bPlace.remove(bot)
            flap_group = self.svg.getElementById("flap_{}".format(i+1))
            if(flap_group is not None):
                top_char = flap_group.xpath("svg:path[@layer='top']")
                bot_char = flap_group.xpath("svg:path[@layer='bot']")
                if(len(top_char)):
                    layer_tPlace.append(top_char[0])
                if(len(bot_char)):
                    layer_bPlace.append(bot_char[0])
            # export
            file_name = "{}_{}".format(self.options.file_name,i+1)
            svg_file = inkex.command.write_svg(self.svg, self.options.output_path, file_name + '.svg')
            inkex.command.inkscape(svg_file, export_area_page=True, export_filename=os.path.join(self.options.output_path, file_name))
        return

if __name__ == '__main__':
    a = FlapSplit()
    a.run()

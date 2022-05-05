#!/usr/bin/env python
"""Flapgen path extension (modify path)"""

import math
import inkex
import os
import copy
from lxml import etree
from subprocess import Popen, PIPE
# from .elements._base import load_svg

class FlapGen(inkex.EffectExtension):
    def add_arguments(self, pars):
        pars.add_argument("-r", "--ratio", type=float, default=2.3, help="Flap To Font Scale Factor")
        pars.add_argument("-t", "--top_spacing", type=float, default=6, help="Char Top Spacing")
        pars.add_argument("-m", "--mid_cutout", type=float, default=0.5, help="Clearance between upper and lower flap.")
        pars.add_argument("-o", "--charset_offset", type=int, default=0, help="Flap Set Offset")
        pars.add_argument("-s", "--charset", type=str, default=' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789€$!?.,:/&@#')
        pars.add_argument("-e", "--export", type=bool, default=True)
        pars.add_argument("-p", "--output_path", type=str)
        pars.add_argument("-n", "--file_name", type=str, default='Split_flap_Silkscreen')
        pars.add_argument("-c", "--color", type=bool, default=True)

    def create_char(self,flap_id):  
        inverted = True if 'inverted' in self.svg.getElementById(flap_id).attrib else False

        # Calculate font size
        font_size = round(self.svg.getElementById(flap_id).bounding_box().height * self.options.ratio)
        char_style =  {'style':"font-size:"+str(font_size)+"px;font-family:'Red Hat Mono';fill:#000000;stroke-width:0"}

        # Create Chars 
        top_char = etree.SubElement(self.svg.getElementById("layer_ss_top"), inkex.addNS('text','svg'), char_style )
        top_tspan = etree.SubElement(top_char, inkex.addNS('text','svg'))
        top_tspan.text = self.options.charset[::-1][self.options.charset_offset+1] 
        top_char_id = top_char.get_id()
        if(inverted):
            top_char_flip = inkex.Transform("scale(1, -1)")
            top_char.set('transform', top_char_flip)
        top_char_is_visible = self.options.charset[::-1][self.options.charset_offset+1] != " "

        bot_char = etree.SubElement(self.svg.getElementById("layer_ss_bot"), inkex.addNS('text','svg'), char_style )
        bot_tspan = etree.SubElement(bot_char, inkex.addNS('text','svg'))
        bot_tspan.text = self.options.charset[::-1][self.options.charset_offset]
        bot_char_id = bot_char.get_id()
        if(not inverted):
            bot_char_flip = inkex.Transform("scale(1, -1)")
            bot_char.set('transform', bot_char_flip)
        bot_char_is_visible = self.options.charset[::-1][self.options.charset_offset] != " "

        # Convert chars to path
        self.document = inkex.load_svg(inkex.command.inkscape_command(
            self.svg,
            select=bot_char_id+","+top_char_id,
            verbs=["ObjectToPath","SelectionUnGroup"]))  
        self.svg = self.document.getroot()

        # Move Chars 
        if(top_char_is_visible):
            top_char = self.svg.getElementById("layer_ss_top").getchildren()[-1]
            top_orig_x = top_char.bounding_box().center_x
            top_orig_y = top_char.bounding_box().top if not inverted else top_char.bounding_box().bottom
            top_dest_x = self.svg.getElementById(flap_id).bounding_box().center_x 
            top_dest_y = self.svg.getElementById(flap_id).bounding_box().top + self.options.top_spacing if not inverted else self.svg.getElementById(flap_id).bounding_box().bottom - self.options.top_spacing
            top_char_translate = inkex.Transform('translate(%.2f, %.2f)' % (top_dest_x-top_orig_x, top_dest_y-top_orig_y))
            top_char.set('transform', top_char_translate)

        if(bot_char_is_visible):
            bot_char = self.svg.getElementById("layer_ss_bot").getchildren()[-1]
            bot_orig_x = bot_char.bounding_box().center_x
            bot_orig_y = bot_char.bounding_box().bottom if not inverted else bot_char.bounding_box().top
            bot_dest_x = self.svg.getElementById(flap_id).bounding_box().center_x
            bot_dest_y = self.svg.getElementById(flap_id).bounding_box().bottom  + self.svg.getElementById(flap_id).bounding_box().height - self.options.top_spacing + self.options.mid_cutout if not inverted else self.svg.getElementById(flap_id).bounding_box().top - self.svg.getElementById(flap_id).bounding_box().height + self.options.top_spacing - self.options.mid_cutout
            bot_char_translate = inkex.Transform('translate(%.2f, %.2f)' % (bot_dest_x-bot_orig_x, bot_dest_y-bot_orig_y))
            bot_char.set('transform', bot_char_translate)

        # Create a copy of the flaps a cut tool
        flap_elem = self.svg.getElementById(flap_id)
        if(top_char_is_visible):
            flap_copy_top_elem = copy.deepcopy(flap_elem)
            flap_copy_top_elem.set_id("flap_copy_top")
            self.svg.getElementById("layer_ss_top").append(flap_copy_top_elem)
        if(bot_char_is_visible):
            flap_copy_bot_elem = copy.deepcopy(flap_elem)
            flap_copy_bot_elem.set_id("flap_copy_bot")
            self.svg.getElementById("layer_ss_bot").append(flap_copy_bot_elem)

        # Intersect Char and Flap
        if(top_char_is_visible):
            self.document = inkex.load_svg(inkex.command.inkscape_command(
                self.svg,
                select="flap_copy_top"+","+top_char.get_id(),
                verbs=["SelectionIntersect"]))  
            self.svg = self.document.getroot()

        if(bot_char_is_visible):
            self.document = inkex.load_svg(inkex.command.inkscape_command(
                self.svg,
                select="flap_copy_bot"+","+bot_char.get_id(),
                verbs=["SelectionIntersect"]))  
            self.svg = self.document.getroot()

        if inverted:
            if(top_char_is_visible):
                top_char_element = self.svg.getElementById(top_char.get_id())
                if top_char_element is not None:
                    self.svg.getElementById("layer_ss_bot").append(self.svg.getElementById(top_char.get_id()))
            if(bot_char_is_visible):
                bot_char_element = self.svg.getElementById(bot_char.get_id())
                if bot_char_element is not None:
                    self.svg.getElementById("layer_ss_top").append(self.svg.getElementById(bot_char.get_id()))
        return 

    def effect(self):
        selected_ids = self.options.ids

        self.options.charset+= self.options.charset[0]

        for i in range(len(self.options.charset)-2,-1, -1):
            layer1 = self.svg.getElementById("layer1")

            layer_ss_bot = self.svg.getElementById("layer_ss_bot")
            if(layer_ss_bot is None):  
                layer_ss_bot_atr={}
                layer_ss_bot_atr['id'] = 'layer_ss_bot'
                layer_ss_bot_atr["{http://www.inkscape.org/namespaces/inkscape}groupmode"] = "layer"
                layer_ss_bot_atr["{http://www.inkscape.org/namespaces/inkscape}label"] = "bPlace"
                layer_ss_bot = etree.SubElement(self.svg, inkex.addNS('g','svg'), layer_ss_bot_atr )

            layer_ss_top = self.svg.getElementById("layer_ss_top")
            if(layer_ss_top is None):  
                layer_ss_top_atr={}
                layer_ss_top_atr['id'] = 'layer_ss_top'
                layer_ss_top_atr["{http://www.inkscape.org/namespaces/inkscape}groupmode"] = "layer"
                layer_ss_top_atr["{http://www.inkscape.org/namespaces/inkscape}label"] = "tPlace"
                layer_ss_top = etree.SubElement(self.svg, inkex.addNS('g','svg'), layer_ss_top_atr )
            
            # Clear layers
            for top in self.svg.xpath("//svg:g[@id='layer_ss_top']/*"):
                layer_ss_top.remove(top)
            for bot in self.svg.xpath("//svg:g[@id='layer_ss_bot']/*"):
                layer_ss_bot.remove(bot)


            flap_ids = []
            for flap in self.svg.xpath("//svg:path[@flap='True']"):
                flap_ids.append(flap.get_id())

            for id in flap_ids:
                self.create_char(id)

            if self.options.color:
                for top in self.svg.xpath("//svg:g[@id='layer_ss_top']/*"):
                    top.style['fill'] = "#ff5599"
                for bot in self.svg.xpath("//svg:g[@id='layer_ss_bot']/*"):
                    bot.style['fill'] = "#5f5fd3"

            if self.options.export:
                if os.path.isdir(self.options.output_path):
                    file_name = "{}_[{}]".format(self.options.file_name,self.options.charset_offset)
                    # self.options.file_name + " " + self.options.charset[::-1][self.options.charset_offset+1] + self.options.charset[::-1][self.options.charset_offset] 
                    svg_file = inkex.command.write_svg(self.svg, self.options.output_path, file_name + '.svg')
                    # inkex.command.inkscape(svg_file, export_id="layer_ss_top", export_area_page=True, export_id_only=True, export_filename=os.path.join(self.options.output_path, file_name + '.tPlace.svg'))
                    # inkex.command.inkscape(svg_file, export_id="layer_ss_bot", export_area_page=True, export_id_only=True, export_filename=os.path.join(self.options.output_path, file_name + '.bPlace.svg'))
            self.options.charset_offset = i
        return

if __name__ == '__main__':
    a = FlapGen()
    a.run()

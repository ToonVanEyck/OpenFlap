#!/usr/bin/env python
"""Flapgen path extension (modify path)"""

import math
import inkex
import inkex.command
import os
import copy
from lxml import etree
from subprocess import Popen, PIPE
# from .elements._base import load_svg

class FlapGen(inkex.EffectExtension):
    def add_arguments(self, pars):
        pars.add_argument("-r", "--ratio", type=float, default=2.3, help="Flap To Font Scale Factor")
        pars.add_argument("-t", "--bot_spacing", type=float, default=6, help="Char Bot Spacing")
        pars.add_argument("-m", "--mid_cutout", type=float, default=0.5, help="Clearance between upper and lower flap.")
        pars.add_argument("-o", "--characterMap_offset", type=int, default=0, help="Flap Set Offset")
        pars.add_argument("-s", "--characterMap", type=str, default=' ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789€$!?.,:/@#&')

    def create_char(self,proto_flap,i,char,layer):  

        flap = etree.SubElement(layer, inkex.addNS('g','svg'))
        flap.set_id("flap_{}".format(i))

        # Create top copy of prototype flap
        top_flap_elem = copy.deepcopy(proto_flap)
        top_flap_elem.set_id("top_flap_{}".format(i))
        flap.append(top_flap_elem)
        # Position top copy of prototype flap
        top_orig_x = top_flap_elem.bounding_box().center_x
        top_orig_y = top_flap_elem.bounding_box().center_y
        top_dest_x = proto_flap.bounding_box().center_x + ((proto_flap.bounding_box().width + 1) * i)
        top_dest_y = proto_flap.bounding_box().center_y
        top_char_translate = inkex.Transform('translate(%.2f, %.2f)' % (top_dest_x-top_orig_x, top_dest_y-top_orig_y))
        # top_flap_elem.set('transform', top_char_translate)
        # Create bot copy of prototype flap
        bot_flap_elem = copy.deepcopy(proto_flap)
        bot_flap_elem.set_id("bot_flap_{}".format(i))
        flap.append(bot_flap_elem)
        # Position bot copy of prototype flap
        bot_orig_x = bot_flap_elem.bounding_box().center_x
        bot_orig_y = bot_flap_elem.bounding_box().center_y
        bot_dest_x = proto_flap.bounding_box().center_x #+ ((proto_flap.bounding_box().width + 1) * i)
        bot_dest_y = proto_flap.bounding_box().center_y - 2*proto_flap.bounding_box().height - self.options.mid_cutout
        bot_char_translate = inkex.Transform("scale(1, -1)") * inkex.Transform('translate(%.2f, %.2f)' % (bot_dest_x-bot_orig_x, bot_dest_y-bot_orig_y))
        bot_flap_elem.set('transform', bot_char_translate)

        # Calculate font size
        font_size = round(proto_flap.bounding_box().height * self.options.ratio)
        char_attr =  {'style':"font-size:"+str(font_size)+"px;font-family:'Red Hat Mono';font-weight:600;text-align:center;text-anchor:middle;fill:#000000;stroke-width:0",'x':str(proto_flap.bounding_box().center_x ),'y':str(proto_flap.bounding_box().height * 2 + self.options.mid_cutout - self.options.bot_spacing)}

        # Create character 
        flap_char = etree.SubElement(flap, inkex.addNS('text','svg'), char_attr)
        flap_char.set_id("flap_char_{}".format(i))
        flap_char.text = char 

        # translate group
        flap.set('transform',inkex.Transform("translate({})".format((proto_flap.bounding_box().width + 1) * i)))

        return 

    def effect(self):
        proto_flap = self.svg.xpath("//svg:path[@flap='True']")[0]
        proto_flap.set('mid_cutout',str(self.options.mid_cutout))

        self.options.characterMap+= self.options.characterMap[0]

        tmp_layer = self.svg.getElementById("layer_gen_characterMap")
        
        # Clear layers
        for e in self.svg.xpath("//svg:g[@id='layer_gen_characterMap']/*"):
            tmp_layer.remove(e)

        if(tmp_layer is None):  
            tmp_layer_atr={}
            tmp_layer_atr['id'] = 'layer_gen_characterMap'
            tmp_layer_atr["{http://www.inkscape.org/namespaces/inkscape}groupmode"] = "layer"
            tmp_layer_atr["{http://www.inkscape.org/namespaces/inkscape}label"] = "gen_flapset"
            tmp_layer = etree.SubElement(self.svg, inkex.addNS('g','svg'), tmp_layer_atr )

        for i in range(0,len(self.options.characterMap)-1,1):
            layer1 = self.svg.getElementById("layer1")
            inkex.utils.debug("{}: [{}]".format(i,self.options.characterMap[i]))

            self.create_char(proto_flap,i+1,self.options.characterMap[i],tmp_layer)
        return

if __name__ == '__main__':
    a = FlapGen()
    a.run()

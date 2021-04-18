#!/usr/bin/env python3
#
# Program to configure (graphically) Google Chromebook Pixel 2013 backlight and key lighting
#  found at http://github.com/alfille/pixel2013
#  Note that setuid programs p2013dim must be installd=ed to make the actual changes 
#
# 2021 Paul H Alffille

try:
    import tkinter as tk
    from tkinter import ttk
    from tkinter import font
except:
    print( "Cannot load TKinter (graphics) module, which should be part of the python standard distribution\n" )
    raise

try:
    import sys
except:
    print( "Cannot load sys module, which should be part of the python standard distribution\n" )
    raise

try:
    import signal
except:
    print( "Cannot load signal module, which should be part of the python standard distribution\n" )
    raise

try:
    from pathlib import Path
except:
    print( "Cannot load pathlib/Path module, which should be part of the python standard distribution\n" )
    raise

def signal_handler(signal, frame):
    sys.exit(0)

mainwindow = None
iconfile = "/usr/share/icons/hicolor/64x64/backlighter.png"

class device:
    
    basedir = "/sys/class/"
    
    def __init__( self, devdir ):
        self.choice = None
        self.choices = []
        self.syspath = Path( type(self).basedir ) . joinpath( devdir )
        if self.syspath.exists():
            self.choices = [ d for d in self.syspath.iterdir() if d.is_dir() and d.joinpath('brightness').exists() and d.joinpath('max_brightness').exists() ]
            self.default()

    def default( self ):
        if self.choices == []:
            # ERROR no choices
            self.choice = None
            return
        for d in self.choices:
            t = d.joinpath('type')
            if t.exists() and t.read_text().strip('\n') =='raw' :
                self.control = d.stem
                return
        self.control = self.choices[0].stem

    @property
    def max( self ):
        if self.choice is None:
            return 1
        return self._max

    @property
    def brightness( self ):
        if self.choice is None:
            return 1
        return int( self.bright.read_text().strip('\n') )
    
    @brightness.setter
    def brightness( self, b ):
        if self.choice is not None:
            br = int( float(b) + .5  ) # for rounding
            if br > self._max:
                br = self._max
            if br < 0:
                br = 0
            self.bright.write_text( str(int(br)) ) 

    @property
    def control( self ):
        if self.choice is None:
            return None
        return self.choice.stem

    @control.setter
    def control( self, stem ):
        if stem not in [ d.stem for d in self.choices ]:
            # ERROR bad choice, set to default
            return self.default()
        self.choice = Path( self.syspath ) . joinpath( stem )
        self._max = int( self.choice.joinpath('max_brightness').read_text().strip('\n') )
        if self._max > 5 and self._max < 20:
            self._delta = 1
        else:
            self._delta = self._max / 15.
        self.bright = self.choice.joinpath('brightness')
        
    @property
    def delta( self ):
        return self._delta

    @property
    def controllist( self ):
        if self.choice is None:
            return []
        return [ d.stem for d in self.choices ]

    @property
    def title( self ):
        return type(self).tabtitle
    
class backlight(device):
    tabtitle="Screen backlight"
    def __init__( self ):
        super().__init__("backlight")

class leds(device):
    tabtitle="Key backlight"
    def __init__( self ):
        super().__init__("leds")
        # further restrict choices
        self.choices = [ c for c in self.choices if "lock" not in c.stem ]
        # reselect default
        self.default()

class tab:
    tabcontrol = None
    buttonfont = None
    
    def __init__( self, dev ):
        self.device = dev

        # Notebook if doesn't exist
        if type(self).tabcontrol is None :
            global mainwindow
            type(self).tabcontrol = ttk.Notebook( mainwindow )

        # This Tab
        self.tab = ttk.Frame( type(self).tabcontrol )
        type(self).tabcontrol.add(self.tab, text = self.device.title )
        type(self).tabcontrol.pack( expand=1, fill="both" )
        
        # create empty widget names
        self.bad = None
        self.scale = None
        self.combo = None
        self.plus = None
        self.minus = None
        self.controlvar = tk.StringVar()

        self.control_panel()
                
    def control_panel( self ):
        for w in [ self.plus, self.minus, self.bad, self.scale, self.combo ] :
            if w is not None:
                w.destroy()
                w = None

        # Test Control validity
        if self.device.control is None:
            self.bad = ttk.Label( self.tab, text = "{} control not found at {}".format(self.device.title, self.device.syspath) )
            self.bad.pack( expand = 1, fill="both", padx=10, pady=10 )
            return

        # Scale (slider)
        # possibly get settings from init file
        self.scale = tk.Scale( self.tab, from_=0, to=self.device.max, orient="horizontal", resolution=self.device.delta, bd=5, width=20, showvalue=0 )
        self.scale.set(self.device.brightness)
        self.scale.config(command=self.setlevel )

        self.combo = ttk.Combobox( self.tab, values=self.device.controllist, state="readonly",exportselection=0,textvariable=self.controlvar )
        self.combo.set(self.device.control)
        self.controlvar.trace( 'w', self.setcontrol )

        if type(self).buttonfont is None:
            plus = tk.Button( self.tab, text="+" )
            buttonfont = font.Font( font=plus.cget("font") ).actual()
            type(self).buttonfont = font.Font( family=buttonfont['family'], weight='bold', size=4*buttonfont['size'] )
            plus.destroy()
        
        self.plus = tk.Button( self.tab, text="+", font=type(self).buttonfont, command=self.plusbutton )
        self.minus = tk.Button( self.tab, text="-", font=type(self).buttonfont, command=self.minusbutton )
        
        self.plus.pack( expand=1, fill="y", padx=2, pady=2, side="right")
        self.minus.pack( expand=1, fill="y", padx=2, pady=2, side="left")
        self.scale.pack( expand=1, fill="both", padx=5, pady=5 )
        self.combo.pack( expand=1, side="bottom", fill="x" ,padx=5, pady=5 )

    def setcontrol( self, *args ):
        self.device.control = self.combo.get()
        self.control_panel()

    def plusbutton( self, *args ):
        self.scale.set( self.device.brightness + self.device.delta )

    def minusbutton( self, *args ):
        self.scale.set( self.device.brightness - self.device.delta )

    def setlevel( self, val ):
        self.device.brightness = val
        
def main(args):
    global mainwindow
    global iconfile
    
    # keyboard interrupt
    signal.signal(signal.SIGINT, signal_handler)

    mainwindow = tk.Tk()
    mainwindow.title("Laptop Brightness")
    if Path(iconfile).exists():
        photo = tk.PhotoImage(file = iconfile)
        mainwindow.iconphoto(False, photo)
    mainwindow.resizable(True,True)    
    tab(backlight())
    tab(leds())
    mainwindow.mainloop()

if __name__ == "__main__":
    # execute only if run as a script
    sys.exit(main(sys.argv))

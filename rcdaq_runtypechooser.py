#!/usr/bin/env python3
import subprocess
import tkinter as tk

TIME_USED = 2  # seconds between updates

color1 = "#CCCC99"
buttonbgcolor = "#33CCCC"
oncolor = "orange2"
neutralcolor = "khaki"
slinebg = "#cccc00"

titlefontsize = 15
fontsize = 13
subtitlefontsize = 11
smallfont = ['arial', subtitlefontsize]
normalfont = ['arial', fontsize]
bigfont = ['arial', titlefontsize, 'bold']


def get_runtypes():
    """{name: description}, or None if rcdaq_client can't reach the server
    (as opposed to a successful call that just reports zero runtypes defined)"""
    result = subprocess.run(['rcdaq_client', 'daq_list_runtypes'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        return None
    types = {}
    for line in result.stdout.decode().splitlines():
        parts = line.split()
        if len(parts) >= 3:
            types[parts[0]] = parts[2]
    return types


class RuntypeChooser(tk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.types = {}

        parent.title("Run Type Control")

        self.sline = tk.Label(parent, text="Run Type Control", bg=slinebg, font=bigfont)
        self.sline.pack(side='top', fill='x', ipadx='15m', ipady='1m')

        self.outer = tk.Label(parent, bg=color1, relief='raised')
        self.outer.pack(side='left', fill='x', ipadx='15m')

        self.label_a = tk.Label(self.outer, bg=color1, relief='raised')
        self.label_a.pack(side='top', fill='x')
        self.label_c = tk.Label(self.outer, bg=color1, relief='raised')
        self.label_c.pack(side='bottom', fill='x')
        self.label_b = tk.Label(self.outer, bg=color1, relief='raised')
        self.label_b.pack(side='bottom', fill='x')

        self.currenttypelabel = tk.Label(self.label_a, font=bigfont, fg='red', bg=neutralcolor)
        self.currenttypelabel.pack(side='top', fill='x', expand=True, ipadx='3m', ipady='3m')

        self.currentnamelabel = tk.Label(self.label_b, font=smallfont, fg='black', bg=neutralcolor)
        self.currentnamelabel.pack(side='top', fill='x', expand=True, ipadx='2m', ipady='2m')

        self.buttons = {}
        self.update()

    def set_type(self, name):
        subprocess.run(['rcdaq_client', 'daq_set_runtype', name], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def rebuild_buttons(self, types):
        for button in self.buttons.values():
            button.destroy()
        self.buttons = {}
        for name in sorted(types):
            button = tk.Button(self.label_c, bg=buttonbgcolor, text=name, relief='raised', font=normalfont,
                                command=lambda name=name: self.set_type(name))
            button.pack(side='left', fill='x', expand=True, ipadx='1m', ipady='1m')
            self.buttons[name] = button

    def update(self):
        types = get_runtypes()

        if types is None:
            self.types = {}
            self.rebuild_buttons({})
            self.currenttypelabel.configure(text="RCDAQ not running")
            self.currentnamelabel.configure(text="")
        else:
            if types.keys() != self.types.keys():
                self.rebuild_buttons(types)
            self.types = types

            result = subprocess.run(['rcdaq_client', 'daq_get_runtype'],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            text = result.stdout.decode().strip()

            for button in self.buttons.values():
                button.configure(bg=buttonbgcolor)

            if text == "":
                self.currenttypelabel.configure(text="( no run type set )")
                self.currentnamelabel.configure(text="")
            elif text in self.types:
                self.currenttypelabel.configure(text=text)
                self.currentnamelabel.configure(text=self.types[text])
                self.buttons[text].configure(bg=oncolor)
            else:
                # unrecognized response - most likely the server dropped
                # between the two calls above
                self.currenttypelabel.configure(text="RCDAQ not running")
                self.currentnamelabel.configure(text="")

        self.after(TIME_USED * 1000, self.update)


root = tk.Tk()
app = RuntypeChooser(root)
app.pack(fill='both', expand=True)
root.mainloop()

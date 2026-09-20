#!/usr/bin/env python3
#
# Also runs as "rcdaq_status.py" (a symlink to this file, matching how the
# old rcdaq_control.pl/rcdaq_status.pl pair worked): the status-only mode
# has no action buttons, so it's safe to leave sitting on a desktop to
# watch a remote experiment without any danger of an inadvertent click
# interfering with data taking. The mode is picked by which name the
# script was invoked as.

import argparse
import os
import subprocess
import sys
import tkinter as tk

IS_STATUS_ONLY = os.path.basename(sys.argv[0]).startswith("rcdaq_status")

parser = argparse.ArgumentParser()
parser.add_argument('--display', help="display on the remote display, e.g. over.head.display:0")
parser.add_argument('--geometry', help="standard X11 geometry, e.g. +200+400")
parser.add_argument('--small', action='store_true', help="small size, to sit unobtrusively on a desktop")
parser.add_argument('--large', action='store_true', help="size increase, for showing on an overhead display")
parser.add_argument('--Large', action='store_true', help="size increase, for showing on an overhead display")
parser.add_argument('--huge', action='store_true', help="size increase, for showing on an overhead display")
parser.add_argument('--Huge', action='store_true', help="size increase, for showing on an overhead display")
args = parser.parse_args()

if args.display:
    os.environ['DISPLAY'] = args.display

TIME_USED = 2  # seconds between status polls

# same size table as the old rcdaq_status.pl --small/--large/--Large/--huge/--Huge
SIZES = {
    'Huge':    dict(ipadx='90m', ipady='45m', padx='15m', pady='15m', title=90, normal=60, small=50),
    'huge':    dict(ipadx='70m', ipady='35m', padx='12m', pady='12m', title=70, normal=50, small=40),
    'Large':   dict(ipadx='50m', ipady='24m', padx='8m',  pady='8m',  title=50, normal=35, small=30),
    'large':   dict(ipadx='35m', ipady='18m', padx='5m',  pady='5m',  title=30, normal=27, small=22),
    'small':   dict(ipadx='10m', ipady='5m',  padx='1m',  pady='1m',  title=8,  normal=7,  small=6),
    'default': dict(ipadx='15m', ipady='8m',  padx='1m',  pady='1m',  title=13, normal=12, small=10),
}
if args.Huge:
    size = SIZES['Huge']
elif args.huge:
    size = SIZES['huge']
elif args.Large:
    size = SIZES['Large']
elif args.large:
    size = SIZES['large']
elif args.small:
    size = SIZES['small']
else:
    size = SIZES['default']

ipadx, ipady, padx, pady = size['ipadx'], size['ipady'], size['padx'], size['pady']
smallfont = ['arial', size['small']]
normalfont = ['arial', size['normal']]
bigfont = ['arial', size['title'], 'bold']

color1 = "#CCCC99"
okcolor = "#00cc99"
buttonbgcolor = "#33CCCC"
graycolor = "#666666"
slinebg = "#cccc00"
sline2bg = "#bbbb00"
neutralcolor = "khaki"


def get_name():
    result = subprocess.run(['rcdaq_client', 'daq_getname'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        return " "
    return result.stdout.decode().strip()


class RcdaqControl(tk.Frame):
    def __init__(self, parent):
        super().__init__(parent)
        self.parent = parent
        self.old_run = -1
        self.prev_events = 0
        self.run = -1
        self.openflag = 0

        parent.title("RCDAQ Status" if IS_STATUS_ONLY else "RCDAQ Control")

        self.sline = tk.Label(parent, text="RCDAQ Status" if IS_STATUS_ONLY else "RCDAQ Control",
                               bg=slinebg, font=bigfont)
        self.sline.pack(side='top', fill='x', ipadx=ipadx, ipady=pady)

        self.sline2 = tk.Label(parent, text=get_name(), bg=sline2bg, font=smallfont)
        self.sline2.pack(side='top', fill='x', ipadx='15m', ipady='0.2m')

        outer = tk.Label(parent, bg=color1, relief='raised')
        outer.pack(side='left', fill='x', ipadx=ipadx)
        panel = tk.Label(outer, bg=color1)
        panel.pack(side='top', fill='x', padx=padx, pady=pady)

        self.runstatuslabel = tk.Label(panel, text="Status", font=bigfont, fg='red',
                                        bg=neutralcolor, relief='raised')
        self.runstatuslabel.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.runnumberlabel = tk.Label(panel, text="runnumber", font=normalfont, bg=okcolor, relief='raised')
        self.runnumberlabel.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.eventcountlabel = tk.Label(panel, text="eventcount", font=normalfont, bg=okcolor, relief='raised')
        self.eventcountlabel.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.volumelabel = tk.Label(panel, text="volume", font=normalfont, bg=okcolor, relief='raised')
        self.volumelabel.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.filenamelabel = tk.Label(panel, text="", font=normalfont, bg=okcolor, relief='raised')
        self.filenamelabel.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.button_open = None
        self.button_begin = None
        if not IS_STATUS_ONLY:
            self.button_open = tk.Button(panel, bg=buttonbgcolor, text="Open", font=normalfont,
                                          command=self.open_handler)
            self.button_open.pack(side='top', fill='x', ipadx=padx, ipady=pady)

            self.button_begin = tk.Button(panel, bg=buttonbgcolor, text="Begin", font=normalfont,
                                           command=self.begin_handler)
            self.button_begin.pack(side='top', fill='x', ipadx=padx, ipady=pady)

        self.update()

    def open_handler(self):
        if self.openflag:
            subprocess.run(['rcdaq_client', 'daq_close'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
            subprocess.run(['rcdaq_client', 'daq_open'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def begin_handler(self):
        if self.run < 0:
            subprocess.run(['rcdaq_client', 'daq_begin'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
            subprocess.run(['rcdaq_client', 'daq_end'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def update(self):
        result = subprocess.run(['rcdaq_client', 'daq_status', '-s'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        name = " "
        run_text = evt_text = vol_text = "n/a"
        rate = 0.0

        if result.returncode != 0:
            self.runstatuslabel.configure(text="RCDAQ not running")
            self.run = -1
            self.openflag = 0
            self.prev_events = 0
        else:
            res = result.stdout.decode()
            # e.g. "2 20 0.000610352 0 0 2  rcdaq-00000002-0000.evt  \"ONE\""
            # run, evt, volume, open_flag, server_flag, duration, filename
            vals = res.split(' ')
            run = int(vals[0])
            evt = int(vals[1])
            vol = vals[2]
            openflag = int(vals[3])
            serverflag = int(vals[4])
            duration = vals[5] if len(vals) > 5 else "0"
            fn = vals[6] if len(vals) > 6 else ""
            quoted = res.split('"')
            name = quoted[1] if len(quoted) > 1 else " "

            self.run = run
            self.openflag = openflag
            run_text = str(run)
            evt_text = str(evt)
            vol_text = vol
            rate = (evt - self.prev_events) / TIME_USED
            self.prev_events = evt

            if run < 0:
                if not IS_STATUS_ONLY:
                    self.button_begin.configure(text="Begin", command=self.begin_handler)
                    self.button_open.configure(bg=buttonbgcolor,
                                                text="Close" if openflag else "Open")

                if self.old_run < 0:
                    self.runstatuslabel.configure(text="Stopped")
                else:
                    self.runstatuslabel.configure(text=f"Stopped   Run {self.old_run}")

                if openflag == 1:
                    if serverflag == 1:
                        self.filenamelabel.configure(text="Logging enabled (Server)")
                    else:
                        self.filenamelabel.configure(text="Logging enabled")
                else:
                    self.filenamelabel.configure(text="Logging Disabled")
            else:
                self.old_run = run
                if not IS_STATUS_ONLY:
                    self.button_begin.configure(text="End", command=self.begin_handler)
                    self.button_open.configure(bg=graycolor)

                self.runstatuslabel.configure(text=f"Running for {duration} s")

                if openflag == 1:
                    if serverflag == 1:
                        self.filenamelabel.configure(text=f"File on Server: {fn}")
                    else:
                        self.filenamelabel.configure(text=f"File: {fn}")
                else:
                    self.filenamelabel.configure(text="Logging Disabled")

        self.sline2.configure(text=name)
        self.runnumberlabel.configure(text=f"Run:    {run_text}")
        self.eventcountlabel.configure(text=f"Events: {evt_text} ({rate:.1f} Hz)")
        self.volumelabel.configure(text=f"Volume: {vol_text} MB")

        self.after(TIME_USED * 1000, self.update)


root = tk.Tk()
if args.geometry:
    root.geometry(args.geometry)

app = RcdaqControl(root)
app.pack(fill='both', expand=True)
root.mainloop()

#! /usr/bin/perl

use Tk;

use Getopt::Long qw(:config no_ignore_case);
GetOptions('help','display:s','geometry:s','small','large', 'Large','huge','Huge');

if ($opt_help)
{
    print"
    hv_display.pl [ options ... ]
    --help                      this text
    --display=<remote_display>  display on the remote display, e.g over.head.display:0.
    --geometry=<geometry>       set a geometry, e.g. +200+400
    --small                     make the display small if you want it to just sit unobtrusively on your desktop
    --large --Large --{hH}uge   size increases suitable for showing on an overhead display (following the Latex conventions)

\n";
    exit(0);
}

if ($opt_display)
{
    $ENV{"DISPLAY"} = $opt_display;
}

$time_used = 2;

$name =  `rcdaq_client daq_getname  2>&1`;
my $status = $?;
if ( $status != 0  )
{
    $name=" ";
}


#$color1 = "#cccc99";
#$color1 = "#999900";
$color1 = "#CCCC99";
$okcolor = "#00cc99";
$errorcolor= "#ff6666";
$neutralcolor ="#cc9900";
$panelcolor = "#336633";
$warncolor = "IndianRed4";
$graycolor = "#666666";

$buttonbgcolor='#33CCCC';

$smalltextfg = '#00CCFF';
$smalltextbg = '#333366';

$slinebg='#cccc00';
$sline2bg='#bbbb00';
#$sline2bg='#ffb90f';

$oncolor = "orange2";
$offcolor = "yellow4";


#$bgcolor = "#cccc66";
$bgcolor = "#990000";

$runstatcolor = "aquamarine";
$stopstatcolor = "palegreen";
$neutralcolor = "khaki";

$old_run = -1;

if ($opt_Huge)
{
    $ipadx='90m';
    $ipady='45m';
    $padx='15m';
    $pady='15m';


    $titlefontsize=90;
    $fontsize=60;
    $subtitlefontsize=50;

}
elsif ($opt_huge)
{
    $ipadx='70m';
    $ipady='35m';
    $padx='12m';
    $pady='12m';


    $titlefontsize=70;
    $fontsize=50;
    $subtitlefontsize=40;

}
elsif ($opt_Large)
{
    $ipadx='50m';
    $ipady='24m';
    $padx='8m';
    $pady='8m';


    $titlefontsize=50;
    $fontsize=35;
    $subtitlefontsize=30;

}
elsif ($opt_large)
{
    $ipadx='35m';
    $ipady='18m';
    $padx='5m';
    $pady='5m';


    $titlefontsize=30;
    $fontsize=27;
    $subtitlefontsize=22;

}
elsif ($opt_small)
{
    $ipadx='10m';
    $ipady='5m';
    $padx='1m';
    $pady='1m';


    $titlefontsize=8;
    $fontsize=7;
    $subtitlefontsize=6;

}
else
{
    $ipadx='15m';
    $ipady='8m';
    $padx='1m';
    $pady='1m';

    $titlefontsize=13;
    $fontsize=12;
    $subtitlefontsize=10;
}


$smallfont = ['arial', $subtitlefontsize];
$normalfont = ['arial', $fontsize];
$bigfont = ['arial', $titlefontsize, 'bold'];


$mw = MainWindow->new();

if ($opt_geometry)
{
    $mw->geometry($opt_geometry);
}


$mw->title("RCDAQ Status");


$label{'sline'} = $mw->Label(-text => "RCDAQ Status", -background=>$slinebg, -font =>$bigfont);
$label{'sline'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> $ipadx, -ipady=> $pady);

$label{'sline2'} = $mw->Label(-text => $name, -background=>$sline2bg, -font =>$smallfont);
$label{'sline2'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> '15m', -ipady=> '0.2m');


$frame{'center'} = $mw->Frame()->pack(-side => 'top', -fill => 'x');


$framename = $frame{'center'}->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'left', -fill=> 'x', -ipadx=>$ipadx);


$outerlabel = $framename->Label(-bg => $color1)->pack(-side =>'top', -fill=> 'x', -padx=> $padx,  -pady=> $pady);


$runstatuslabel = $outerlabel->
      Label(-text=> "Status", -font => $bigfont, -fg =>'red', -bg => $neutralcolor)->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);

$runnumberlabel = $outerlabel->
      Label(-text => "runnumber", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);

$eventcountlabel = $outerlabel->
      Label(-text => "eventcount", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);

$volumelabel = $outerlabel->
      Label(-text => "volume", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);

$filenamelabel = $outerlabel->
      Label(-text => "filename", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);



# update once to fill all the values in
$prev_evt = 0;
$delta_evt = 0;
$rate = 0;
update();

#and schedule an update every 5 seconds



$mw->repeat($time_used * 1000,\&update);

MainLoop();




sub update()
{


    my $res = `rcdaq_client daq_status -s 2>&1`;
#    print $res;

    my $status = $?;
    if ( $status != 0  )
    {
	$runstatuslabel->configure(-text =>"RCDAQ not running");
	$run = "n/a";
	$evt =  "n/a";
	$v =  "n/a";
	$duration = 0;
	$name=" ";
    }
    else
    {
	($run, $evt, $v, $openflag, $serverflag, $duration, $fn  )= split (/\s/ ,$res);
	($junk, $name )= split (/\"/ ,$res);
#    print " run $run  evt $evt  vol $v open  $openflag file  $fn \n";
	
	$delta_evt = $evt - $prev_evt;
	$rate = $delta_evt/2;       # assuming 2s update, should use variable in case this changes
	$prev_evt = $evt;

	if ( $run < 0)
	{
	    if ( $old_run < 0)
	    {
		$runstatuslabel->configure(-text =>"Stopped");
	    }
	    else
	    {
		$runstatuslabel->configure(-text =>"Stopped   Run $old_run");
	    }
		
	    if ( $openflag == 1)
	    {		
		if ( $serverflag == 1 )
		{
		    $filenamelabel->configure(-text =>"Logging enabled (Server)");
		}
		else
		{
		    $filenamelabel->configure(-text =>"Logging enabled");
		}
	    }

	    else
	    {
		$filenamelabel->configure(-text =>"Logging Disabled");
	    }

	}
	else
	{
	    $old_run = $run;
	    
	    $runstatuslabel->configure(-text =>"Running for $duration s");
	    
	    if ( $openflag == 1 )
	    {
		if ( $serverflag == 1 )
		{
		    $filenamelabel->configure(-text =>"File on Server: $fn");
		}
		else
		{
		    $filenamelabel->configure(-text =>"File: $fn");
		}
	    }
	    else
	    {
		$filenamelabel->configure(-text =>"Logging Disabled");
	    }

	}
    
    }

    $label{'sline2'}->configure( -text =>  $name);
    $runnumberlabel->configure(-text => "Run:    $run");
    
    $eventcountlabel->configure(-text =>"Events: $evt  ($rate Hz)");
    
    $volumelabel->configure(-text =>    "Volume: $v MB");
    
    
}


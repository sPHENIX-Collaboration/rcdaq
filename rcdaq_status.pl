#! /usr/bin/perl

use Tk;

use Getopt::Long;
GetOptions('help','display:s','geometry:s','large');

if ($opt_help)
{
    print"
    hv_display.pl [ options ... ]
    --help                      this text
    --display=<remote_display>  display on the remote display, e.g over.head.display:0.
    --geometry=<geometry>       set a geometry, e.g. +200+400
    --large                     size suitable for showing on an overhead display

\n";
    exit(0);
}

if ($opt_display)
{
    $ENV{"DISPLAY"} = $opt_display;
}

$time_used = 2;

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

$oncolor = "orange2";
$offcolor = "yellow4";


#$bgcolor = "#cccc66";
$bgcolor = "#990000";

$runstatcolor = "aquamarine";
$stopstatcolor = "palegreen";
$neutralcolor = "khaki";



if ($opt_large)
{

    $ipadx='35m';
    $ipady='18m';
    $padx='5m';
    $pady='5m';


    $titlefontsize=30;
    $fontsize=27;
    $subtitlefontsize=25;
        $smallfont = ['arial', $subtitlefontsize];
    $normalfont = ['arial', $fontsize];
    $bigfont = ['arial', $titlefontsize, 'bold'];

}
else
{
    $ipadx='15m';
    $ipady='8m';
    $padx='1m';
    $pady='1m';

    $titlefontsize=13;
    $fontsize=12;
    $subtitlefontsize=11;
    $smallfont = ['arial', $subtitlefontsize];
    $normalfont = ['arial', $fontsize];
    $bigfont = ['arial', $titlefontsize, 'bold'];

}




$mw = MainWindow->new();

if ($opt_geometry)
{
    $mw->geometry($opt_geometry);
}


$mw->title("RCDAQ Status");


$label{'sline'} = $mw->Label(-text => "RCDAQ Status", -background=>$slinebg, -font =>$bigfont);
$label{'sline'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> $ipadx, -ipady=> $pady);



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
      Label(-text => "filename", -font => $smallfont, -bg => $okcolor, -relief=> 'raised')->
      pack(-side =>'top', -fill=> 'x', -ipadx=> $padx,  -ipady=> $pady);



# update once to fill all the values in
update();

#and schedule an update every 5 seconds



$mw->repeat($time_used * 1000,\&update);

MainLoop();




sub update()
{


    my $res = `rcdaq_client daq_status -s 2>&1`;
#    print $res;

    if ( $res =~ /system error/ )
    {
	$runstatuslabel->configure(-text =>"RCDAQ not running");
	$run = "n/a";
	$evt =  "n/a";
	$v =  "n/a";
	$duration = 0;
    }
    else
    {
	($run, $evt, $v, $openflag, $fn, $duration)= split (/\s/ ,$res);
#    print " run $run  evt $evt  vol $v open  $openflag file  $fn \n";
	
	if ( $run < 0)
	{
	    $runstatuslabel->configure(-text =>"Stopped   Run $old_run");

	    if ( $openflag )
	    {
		
		$filenamelabel->configure(-text =>"Logging enabled");
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
	    
	    if ( $openflag )
	    {
		
		$filenamelabel->configure(-text =>"File: $fn");
	    }
	    else
	    {
		$filenamelabel->configure(-text =>"Logging Disabled");
	    }
	}
    
    }
       
    $runnumberlabel->configure(-text => "Run:    $run");
    
    $eventcountlabel->configure(-text =>"Events: $evt");
    
    $volumelabel->configure(-text =>    "Volume: $v MB");
    
    
}


#! /usr/bin/perl

use Tk;


sub daq_begin 
{

    $result = `rcdaq_client daq_begin`;
}

sub daq_end
{

    $result = `rcdaq_client daq_end`;
}

sub daq_open
{

    $result = `rcdaq_client daq_open`;
}

sub daq_close
{

    $result = `rcdaq_client daq_close`;
}

sub dummy
{

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
$sline2bg='#bbbb00';
#$sline2bg='#ffb90f';

$oncolor = "orange2";
$offcolor = "yellow4";


#$bgcolor = "#cccc66";
$bgcolor = "#990000";

$runstatcolor = "aquamarine";
$stopstatcolor = "palegreen";
$neutralcolor = "khaki";

#$smallfont = "6x10";

$titlefontsize=13;
$fontsize=12;
$subtitlefontsize=10;
$smallfont = ['arial', $subtitlefontsize];
$normalfont = ['arial', $fontsize];
$bigfont = ['arial', $titlefontsize, 'bold'];


$old_run = "-1";

$name =  `rcdaq_client daq_getname  2>&1`;
my $status = $?;
if ( $status != 0  )
{
    $name=" ";
}


$mw = MainWindow->new();

$smallfont = $mw->fontCreate('code', -family => 'fixed',-size => 10);


$mw->title("RCDAQ Control");


$label{'sline'} = $mw->Label(-text => "RCDAQ Control", -background=>$slinebg, -font =>$bigfont);
$label{'sline'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> '15m', -ipady=> '1m');

$label{'sline2'} = $mw->Label(-text => $name, -background=>$sline2bg, -font =>$smallfont);
$label{'sline2'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> '15m', -ipady=> '0.2m');

#$label{'time'} = $mw->Label(-background =>$smalltextbg, -fg=>$smalltextfg, -font =>$normalfont);
#$label{'time'}->pack(-in => $label{'sline'}, -side=> 'left');


$frame{'center'} = $mw->Frame()->pack(-side => 'top', -fill => 'x');


$framename = $frame{'center'}->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'left', -fill=> 'x', -ipadx=>'15m');


$outerlabel = $framename->Label(-bg => $color1)->pack(-side =>'top', -fill=> 'x', -padx=> '1m',  -pady=> '1m');


$runstatuslabel = $outerlabel->
    Label(-text=> "Status", -font => $bigfont, -fg =>'red', -bg => $neutralcolor,  -relief=> 'raised')->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');

$runnumberlabel = $outerlabel->
    Label(-text => "runnumber", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');

$eventcountlabel = $outerlabel->
    Label(-text => "eventcount", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');

$volumelabel = $outerlabel->
    Label(-text => "volume", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');

$filenamelabel = $outerlabel->
    Label(-text => "", -font => $normalfont, -bg => $okcolor, -relief=> 'raised')->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');


$button_open = $outerlabel->
    Button( -bg => $buttonbgcolor, -text => "Open", -command => [\&daq_open,$b],  -relief =>'raised',  -font=> $normalfont)->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');

$button_begin = $outerlabel->
    Button(-bg => $buttonbgcolor, -text => "Begin", -command => [\&daq_begin, $b],  -relief =>'raised',  -font=> $normalfont)->
    pack(-side =>'top', -fill=> 'x', -ipadx=> '1m',  -ipady=> '1m');



# update once to fill all the values in
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
	# 7 292 0.125664 1 2  rcdaq-00000007-0000.evt 6 "mlpvm2"
	($run, $evt, $v, $openflag, $serverflag, $fn, $duration )= split (/\s+/ ,$res);
	($junk, $name )= split (/\"/ ,$res);
	#print " run $run  evt $evt  vol $v open  $openflag file  $fn $name \n";
	
	if ( $run < 0)
	{
	    $button_begin->configure(-text =>"Begin");
	    $button_begin->configure(-command => [\&daq_begin]);
	    
	    if ( $old_run < 0)
	    {
		$runstatuslabel->configure(-text =>"Stopped");
	    }
	    else
	    {
		$runstatuslabel->configure(-text =>"Stopped   Run $old_run");
	    }

	    $button_open->configure(-bg => $buttonbgcolor );
	    
	    if ( $openflag)
	    {
		$button_open->configure(-text =>    "Close");
		$button_open->configure(-command => [\&daq_close] );
	    }
	    else
	    {
		$button_open->configure(-text =>    "Open");
		$button_open->configure(-command => [\&daq_open] );
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
	else  # we are running 
	{
	    $old_run = $run;
	    
	    $button_begin->configure(-text =>"End");
	    $button_begin->configure(-command => [\&daq_end]);
	    
	    $runstatuslabel->configure(-text =>"Running for $duration s");
	    $button_open->configure(-bg => $graycolor );
	    $button_open->configure(-command => [\&dummy] );
	    
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
    
    $eventcountlabel->configure(-text =>"Events: $evt");
    
    $volumelabel->configure(-text =>    "Volume: $v MB");
    
    
}


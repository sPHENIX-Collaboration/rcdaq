#! /usr/bin/perl

use Tk;


sub getList 
{
    open (L, "rcdaq_client daq_list_runtypes |");
    while (<L>)
    {
	chop;
	my ( $name, $dummy, $value) = split;
#	print "$name $value\n";
	$typelist{$name} = 0;
	$namelist{$name} = $value;
    }
    close L;
}

sub set_type
{
    my $t = shift;
    $result = `rcdaq_client daq_set_runtype $t`;
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

$oncolor = "orange2";
$offcolor = "yellow4";


#$bgcolor = "#cccc66";
$bgcolor = "#990000";

$runstatcolor = "aquamarine";
$stopstatcolor = "palegreen";
$neutralcolor = "khaki";


$titlefontsize=15;
$fontsize=13;
$subtitlefontsize=11;
$smallfont = ['arial', $subtitlefontsize];
$normalfont = ['arial', $fontsize];
$bigfont = ['arial', $titlefontsize, 'bold'];



&getList;

$mw = MainWindow->new();



$mw->title("Run Type Control");


$label{'sline'} = $mw->Label(-text => "Run Type Control", -background=>$slinebg, -font =>$bigfont);
$label{'sline'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> '15m', -ipady=> '1m');


$frame{'center'} = $mw->Frame()->pack(-side => 'top', -fill => 'x');


$framename = $frame{'center'}->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'left', -fill=> 'x', -ipadx=>'15m');


$label_a = $framename->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'top', -fill=> 'x');
$label_c = $framename->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'bottom', -fill=> 'x');
$label_b = $framename->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'bottom', -fill=> 'x');


$currenttypelabel = $label_a->
      Label(-font => $bigfont, -fg =>'red', -bg => $neutralcolor)->
      pack(-side =>'top', -fill=> 'x', -expand => 1, -ipadx=> '3m',  -ipady=> '3m');

$currentnamelabel = $label_b->
      Label( -font => $smallfont, -fg =>'black', -bg => $neutralcolor)->
      pack(-side =>'top', -fill=> 'x', -expand => 1, -ipadx=> '2m',  -ipady=> '2m');


foreach $type ( sort keys %typelist )
{
    $button{$type} = $label_c->
	Button( -bg => $buttonbgcolor, -text => $type, -command => [\&set_type,$type],  -relief =>'raised',  -font=> $normalfont)->
	pack(-side =>'left', -fill=> 'x',-expand => 1, -ipadx=>'1m', -ipady=> '1m');

}

# update once to fill all the values in
update();

#and schedule an update every 5 seconds



$mw->repeat($time_used * 1000,\&update);

MainLoop();




sub update()
{


    my $res = `rcdaq_client daq_get_runtype  2>&1`;
#    print $res;

    if ( $res =~ /system error/  || $res =~ /Program not registered/ )
    {
	$currenttypelabel->configure(-text =>"RCDAQ not running");

    }
    else
    {

	foreach $type ( sort keys %typelist )
	{
	    $button{$type}->configure( -bg => $buttonbgcolor);
	}

	chomp $res;
	if ($res eq "")
	{
	    $currenttypelabel->configure(-text => "( no run type set )");
	    $currentnamelabel->configure(-text => "");
	}
	else
	{

	    $currenttypelabel->configure(-text => $res);
	    $currentnamelabel->configure(-text => $namelist{$res});
	    $button{$res}->configure( -bg => $oncolor);
	}    
    }
}

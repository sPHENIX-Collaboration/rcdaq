#! /usr/bin/perl

use Tk;
require Tk::Balloon;

sub display_script 
{
    my $arg=$_[0];
    $mytext->delete('1.0', 'end');
    $mytext->insert('end', $LC{$arg});
    $filelabel->configure ( -text=>$arg, -bg=>$neutralcolor); 
    $executebutton->configure( -command => [\&execute_script,$arg], -bg => $buttonbgcolor, -state => "normal");
    $cancelbutton->configure( -command => [\&cancel_script], -bg => $buttonbgcolor, -state => "normal");
}

sub execute_script 
{
    my $arg=$_[0];
    $mytext->delete('1.0', 'end');
    open (OUT, "/bin/sh $_[0] |");
    while ( <OUT> )
    {
	$mytext->insert('end', $_);
	$mytext->SetCursor('end');
    }
    $filelabel->configure ( -text=>$arg . ". executed", -bg=>$executedcolor); 
    $executebutton->configure(  -bg => $graycolor, -state => "disabled");
    $cancelbutton->configure(  -bg => $graycolor, -state => "disabled");
}

sub cancel_script 
{
    my $arg=$_[0];
    $mytext->delete('1.0','end');
    $filelabel->configure ( -text=>"", -bg=>$graycolor); 
    $executebutton->configure( -bg => $graycolor, -state => "disabled");
    $cancelbutton->configure( -bg => $graycolor, -state => "disabled");

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
$graycolor = "#929292";
$executedcolor="orange";


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

$smallfont = "6x10";

$normalfont = "6x13";
$bigfont = "9x15bold";

$mw = MainWindow->new();

$mw->title("RCDAQ Configuration");


$label{'sline'} = $mw->Label(-text => "RCDAQ Configuration", -background=>$slinebg, -font =>$bigfont);
$label{'sline'}->pack(-side=> 'top', -fill=> 'x', -ipadx=> '15m', -ipady=> '1m');


$frame{'center'} = $mw->Frame()->pack(-side => 'top', -fill => 'x');



# update once to fill all the values in
#update();

getfilelist();

my $file;

my @files  = ( sort keys %SC );

$label{'buttonframe'} = $frame{'center'}->Label(-bg => $color1, -relief=> 'raised')->pack(-side =>'left', -fill=> 'both');

$label{'descriptionframe'} = $frame{'center'}->Label(-bg => $neutralcolor, -relief=> 'raised')->pack(-side =>'right', -fill=> 'x');
$mytext = $label{'descriptionframe'}->Scrolled( 'Text', -scrollbars => 'osoe', -width => 80 )->pack(-side =>'top', -fill=> 'both', -padx=> '1m',  -pady=> '1m');

$executebutton =  $label{'descriptionframe'}->
    Button( -bg => $graycolor, -text => "Execute", -relief =>'raised', -font=> $normalfont)->
	pack(-side =>'right', -fill=> 'x', -ipadx=> '2m',  -ipady=> '1m');

$cancelbutton =  $label{'descriptionframe'}->
    Button( -bg => $graycolor, -text => "Cancel", -relief =>'raised', -font=> $normalfont)->
	pack(-side =>'right', -fill=> 'x', -ipadx=> '2m',  -ipady=> '1m');

$filelabel =  $label{'descriptionframe'}->
    Label( -bg => $graycolor, -text => "", -relief =>'flat', -font=> $normalfont)->
	pack(-side =>'left', -fill=> 'x', -ipadx=> '2m',  -ipady=> '1m');

$label{'outerlabel'} = $label{'buttonframe'}->Label(-bg => $color1)->pack(-side =>'top', -fill=> 'x', -padx=> '1m',  -pady=> '1m');
$label{'toplinelabel'} = $label{'outerlabel'}->Label(-bg => $color1)->pack(-side =>'top', -fill=> 'y', -padx=> '0m',  -pady=> '0m');

$label{'heading'} = $label{'toplinelabel'}->
      Label(-text => "Configurations", -font => $bigfont, -fg => 'black', -bg => $panelcolor)->
      pack(-side =>'top', -fill=> 'x', -ipadx=> '2m',  -ipady=> '1m');

my $b = $mw->Balloon(-statusbar => $frame{'center'});

foreach $filename ( sort keys %SC )
{
    $button{$filename} = $label{'outerlabel'}->
	Button( -bg => $buttonbgcolor, -text => $filename, -command => [\&display_script,$filename], -relief =>'raised', -font=> $normalfont)->
	pack(-side =>'top', -fill=> 'x', -ipadx=> '2m',  -ipady=> '1m');
    
    $b->attach( $button{$filename}, -balloonmsg => $SC{$filename} );
}


#$mw->repeat($time_used * 1000,\&update);

MainLoop();


sub getfilelist()
{

# get all files ending in .sh or pl
    my @list = <*.{sh,pl}>;

    foreach my $f ( @list )
    {
	open(FH, $f);
	my @buffer=<FH>;
	close FH;

	my @lines = grep {/#\s*--RCDAQGUI/} @buffer;
	if ($#lines >=0) 
	{

#     get the "short comment"
	    my @SCT = grep {/#\s*--SC/} @buffer;
	    if ($#SCT >=0)
	    {
		my $SCT=$SCT[ $#SCT ];
		$SCT =~ s/#.--SC//;
		$SC{"$f"} = $SCT;
	    }
	    else
	    {
		$SC{"$f"}=0;
	    }
	    
	    my @LCT = grep {/#\s*--LC/} @buffer;
	    if ($#LCT >=0)
	    {
		my $LCT=join ("", @LCT);
		$LCT =~ s/#\s*--LC//g;
		$LC{"$f"} = $LCT;
	    }
	    else
	    {
		$LC{"$f"}=0;
	    }
	}

    }
    
    #  foreach $f ( sort keys %SC  )
    # {
    # 	print "$f", $SC{$f}, "\n", $LC{$f}, "\n";
    # }
    
}


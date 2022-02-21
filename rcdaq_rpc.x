
const STATUSFORMAT_SHORT  = 0;
const STATUSFORMAT_NORMAL = 1;
const STATUSFORMAT_LONG   = 2;
const STATUSFORMAT_VLONG  = 3;



struct shortResult {
  string str<>;
  int content;
  int what;
  int status;
};

const NIPAR = 16;

struct actionblock {
	int action;
	int ipar[16];
	float  value;
	int spare;
	string spar<>;
	string spar2<>;

};

const NSTRINGPAR = 14;

struct deviceblock {
	int npar;
	string argv0<>;
	string argv1<>;
	string argv2<>;
	string argv3<>;
	string argv4<>;
	string argv5<>;
	string argv6<>;
	string argv7<>;
	string argv8<>;
	string argv9<>;
	string argv10<>;
	string argv11<>;
	string argv12<>;
	string argv13<>;
};



program RCDAQ
{
  version RCDAQ_VERS 
    { 

      shortResult r_action(actionblock) = 1;
      shortResult r_create_device(deviceblock) = 2;
      shortResult r_shutdown() = 3;

    } = 1;

} = 0x3feeeffc;


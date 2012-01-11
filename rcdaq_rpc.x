

struct shortResult {
  string str<>;
  int content;
  int what;
  int status;
};

struct actionblock {
	int action;
	int ipar[16];
	float  value;
	int spare;
	string spar<>;
	string spar2<>;

};

const NSTRINGPAR = 8;

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


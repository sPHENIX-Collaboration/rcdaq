
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



program RCDAQ
{
  version RCDAQ_VERS 
    { 

      shortResult r_action(actionblock) = 1;
      shortResult r_create_device(actionblock) = 2;
      shortResult r_shutdown() = 3;

    } = 1;

} = 0x3feeeffc;


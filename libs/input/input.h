class Input {

	public:
		bool left;
		bool right;
		bool up;
		bool down;
		bool forward;
		bool back;
		void init();
		void update(sensors_event_t);

	private:
		unsigned long lastLeft;
		unsigned long lastRight;
		unsigned long lastUp;
		unsigned long lastDown;
		unsigned long lastForward;
		unsigned long lastBack;

};


void Input::init(){
	this->lastLeft = millis();
	this->lastRight = millis();
	this->lastUp = millis();
	this->lastDown = millis();
	this->lastForward = millis();
	this->lastBack = millis();
};


void Input::update(sensors_event_t event){
	int x = -event.acceleration.x;
	int y = event.acceleration.y;
	int z = event.acceleration.z;
	unsigned long now = millis();

	/*
		X - axis
	*/
	if (!this->left && x < -3 && now > this->lastLeft + 300){
		this->left = true;
		this->lastLeft = now;
	} else {
		this->left = false;
	}

	if (!this->right && x > 3 && now > this->lastRight + 300){
		this->right = true;
		this->lastRight = now;
	} else {
		this->right = false;
	}

	/*
		Y - axis
	*/
	if (!this->up && y < -3 && now > this->lastUp + 300){
		this->up = true;
		this->lastUp = now;
	} else {
		this->up = false;
	}

	if (!this->down && y > 3 && now > this->lastDown + 300){
		this->down = true;
		this->lastDown = now;
	} else {
		this->down = false;
	}

	/*
		Z - axis
	*/
	if (!this->forward && z < 0 && now > this->lastForward + 500 && now > this->lastBack + 500){
		this->left = false;
		this->right = false;
		this->up = false;
		this->down = false;
		this->forward = true;
		this->lastForward = now;
	} else {
		this->forward = false;
	}

	if (!this->back && z > 20 && now > this->lastBack + 500 && now > this->lastForward + 500){
		this->left = false;
		this->right = false;
		this->up = false;
		this->down = false;
		this->back = true;
		this->lastBack = now;
	} else {
		this->back = false;
	}


};
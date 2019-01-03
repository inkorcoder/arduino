float deg2rad(float degrees) {
	return degrees * PI / 180;
};

float rad2deg(float radians) {
	return radians * 180 / PI;
};

class Vector {
	public:

		Vector(float x, float y);

		float x;
		float y;

		int X();
		int Y();

		float Vector::getAngle();
		void Vector::setAngle(float);
		float Vector::getLength();
		void Vector::setLength(float);
		void Vector::add(Vector *);
		void Vector::subtract(Vector *);
		void Vector::multiply(float);
		void Vector::divide(float);
		void Vector::normalize();
		Vector * Vector::getNormal();
		float Vector::distanceTo(Vector *);
		void Vector::clampX(float, float);
		void Vector::clampY(float, float);
		Vector * Vector::clone();
		void Vector::reset();
		bool Vector::is(int, int);
	// private:
};


Vector::Vector(float newX = 0, float newY = 0){
	x = newX;
	y = newY;
};


float Vector::getAngle(){
	return atan2(y, x);
};


void Vector::setAngle(float angle){
	float length = this->getLength();
	float a = deg2rad(angle);
	x = cos(a) * length;
	y = sin(a) * length;
};


float Vector::getLength(){
	return sqrt(x * x + y * y);
};


void Vector::setLength(float length){
	float angle = this->getAngle();
	this->x = cos(angle) * length;
	this->y = sin(angle) * length;
};


void Vector::add(Vector * vec){
	this->x += vec->x;
	this->y += vec->y;
};


void Vector::subtract(Vector * vec){
	this->x -= vec->x;
	this->y -= vec->y;
};


void Vector::multiply(float multiplier){
	this->x *= multiplier;
	this->y *= multiplier;
};


void Vector::divide(float divider){
	this->x /= divider;
	this->y /= divider;
};


void Vector::normalize(){
	float length = this->getLength();
	float x = this->x / length;
	float y = this->y / length;
	this->x = x;
	this->y = y;
}


Vector * Vector::getNormal(){
	Vector * newVector = new Vector(-this->y, this->x);
	return newVector;
};


float Vector::distanceTo(Vector * vec){
	Vector * newVector = new Vector(vec->x - this->x, vec->y - this->y);
	return newVector->getLength();
};


void Vector::clampX(float min, float max){
	this->x = (this->x < min) ? min : (this->x > max) ? max : this->x;
};


void Vector::clampY(float min, float max){
	this->y = (this->y < min) ? min : (this->y > max) ? max : this->y;
};


int Vector::X(){
	return (int)this->x;
};


int Vector::Y(){
	return (int)this->y;
};


Vector * Vector::clone(){
	return new Vector(this->x, this->y);
};


void Vector::reset(){
	this->x = 0;
	this->y = 0;
};


bool Vector::is(int X, int Y){
	return (this->x == X && this->y == Y);
};
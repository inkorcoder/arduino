class ClassicArray {
	public:
		ClassicArray(int length);
		void push(int v) {}

	private:
		int* array;
		int size;
};

ClassicArray::ClassicArray(int length){
	this->array = new int[length];
}

ClassicArray::push(int v){
	this->array[size++] = v;
}
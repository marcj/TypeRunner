
xcode:
	cd xcode && cmake -D CMAKE_C_COMPILER="`xcrun -find cc`" -D CMAKE_CXX_COMPILER="`xcrun -find c++`" .. -GXcode
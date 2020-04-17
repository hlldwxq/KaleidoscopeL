def double func(){			

	var n = 4.4                      # -- the type of variable is only could be double
	n = 3.9                          # -- variable is mutable 
	var x = 2.3 , y = 3.1 , z        # -- several variable could be defined or initiate together	

	# -- there are only two methods to define or initiate static array
	array array1 = [3]               # -- define
	array array2 = {1.1, 2.2}        # -- define and initiate

									 # -- dynamic array and we could not declare a pointer now 
	array *dynamicArray = new [6]    # -- dynamic array define
	dynamicArray[n] = x              # -- initiate
	                                 # -- n is double, but when it is used as array index, 
									 # -- it will be transferd to int 
									 # -- 3.9 will become 3,  3.2 will also become 3

	for i = 0, i < 6, 1.0 in {       # -- 1.0 is step, the default step is 1.0
		dynamicArray[i] = i          # -- if there is only one expression, { } could be omited 
	}
	
	if dynamicArray[n] > x           # -- else cannot be omited
	then { dynamicArray[n] = x }     # -- '{ }' could be omited
	else  dynamicArray[n] = n 

									 # -- the return type could only be double or void
	return dynamicArray[n]           # -- now the return must be at the last of function
}
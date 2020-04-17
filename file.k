# Can I pass parameters?
# def double func(double a) {
#   var n = 4
#   return n
# }

# Are there other types than double
# def int func() {
#   var n = 4
#   return n
# }

# This does not compile?
# def double constfun() {
#   return 0
# }

def double test() {
  var res = 0
  var sign = 1
  var tmp

  var i = 2

  tmp = sign / i    # Something broken here?
  res = res + tmp

  return res
}

def double test2() {
  var n = 50
  array a = [50]
# array a = [n]  Does not work?

  for i=0, i<n-1 in {
    a[i] = i
  }

 var res = 0

 for i=0, i<n-1 in {
   res = res + a[i]
 }

  return res
}



# Using Gregory-Leibniz series
def double pi(){
  var res = 0
 var sign = 1
  var tmp
  for i = 1, i < 5, 2 in {    # Interpretation of < seems to be <=?
    tmp=sign / i
    res=res+tmp
#    # res = res + sign / i   This seems to be translated as (res + sign) / i. Parenthesis do not work.
#    sign=-sign
#    res = res + i
  }

  return res
}


# def double func(){
#
# 	var n = 4.4                      # -- the type of variable is only could be double
# 	n = 3.9                          # -- variable is mutable
# 	var x = 2.3 , y = 3.1 , z        # -- several variable could be defined or initiate together
#
# 	# -- there are only two methods to define or initiate static array
# 	array array1 = [3]               # -- define
# 	array array2 = {1.1, 2.2}        # -- define and initiate
#
# 									 # -- dynamic array and we could not declare a pointer now
# 	array *dynamicArray = new [6]    # -- dynamic array define
# 	dynamicArray[n] = x              # -- initiate
# 	                                 # -- n is double, but when it is used as array index,
# 									 # -- it will be transferd to int
# 									 # -- 3.9 will become 3,  3.2 will also become 3
#
# 	for i = 0, i < 6, 1.0 in {       # -- 1.0 is step, the default step is 1.0
# 		dynamicArray[i] = i          # -- if there is only one expression, { } could be omited
# 	}
#
# 	if dynamicArray[n] > x           # -- else cannot be omited
# 	then { dynamicArray[n] = x }     # -- '{ }' could be omited
# 	else  dynamicArray[n] = n
#
# 									 # -- the return type could only be double or void
# 	return dynamicArray[n]           # -- now the return must be at the last of function
# }

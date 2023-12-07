Path_T1 = ./Term_1
Path_T2 = ./Term_2

T1 :
	g++ -std=c++17 $(Path_T1)/Term_1.cpp -o $(Path_T1)/Term_1.out

T2 :
	g++ -std=c++17 $(Path_T2)/Term_2.cpp -o $(Path_T2)/Term_2.out

.PHONY : T1_us_web
T1_us_web : T1
	./$(Path_T1)/Term_1.out http://hsccl.us.to/index.htm ./web

.PHONY : T1_us_test
T1_us_test : T1
	./$(Path_T1)/Term_1.out http://hsccl.us.to/index.htm ./test

.PHONY : T1_mooo_web
T1_mooo_web : T1
	./$(Path_T1)/Term_1.out http://hsccl.mooo.com/index.htm ./web

.PHONY : T1_mooo_test
T1_mooo_test : T1
	./$(Path_T1)/Term_1.out http://hsccl.mooo.com/index.htm ./test

.PHONY : T2_us_web
T2_us_web : T2
	./$(Path_T2)/Term_2.out http://hsccl.us.to/index.htm ./web

.PHONY : T2_us_test
T2_us_test : T2
	./$(Path_T2)/Term_2.out http://hsccl.us.to/index.htm ./test

.PHONY : T2_mooo_web
T2_mooo_web : T2
	./$(Path_T2)/Term_2.out http://hsccl.mooo.com/index.htm ./web

.PHONY : T2_mooo_test
T2_mooo_test : T2
	./$(Path_T2)/Term_2.out http://hsccl.mooo.com/index.htm ./test

.PHONY : clean
clean :
	@rm -rf ./web/* ./test/*

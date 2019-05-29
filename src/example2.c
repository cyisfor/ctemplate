;|C
int main(void) {
	int foo = 42;
	|;
	int main(void) {
		puts("runtime after template goes here.");
		int bar = ;|c "%d", foo|;;
		printf("in runtime, bar is %d\n", bar);
		printf("in compile time, foo is ;|c "%d", foo|;\n");
		int i;
		for(i=0;i<bar;++i) {
			puts("foo");
		}
		;|C
				int i;
				for(i=0;i<foo;++i) {
					|;puts("bar");;|;
				}
				|;
	}
	;|
		}
|;

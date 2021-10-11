JAVA_HOME=$HOME/repo/dw11/build/linux-x86_64-normal-server-slowdebug/images/jdk
$JAVA_HOME/bin/java -Xlog:jit+inlining=debug -Xbatch -XX:-TieredCompilation -XX:-UseOnStackReplacement -XX:CompileCommandFile=./test.cmd AllocAndCopy 2

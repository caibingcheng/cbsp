function test() {
    CONT_LENGTH=10000
    FILE_LENGTH=1000

    make debug

    TEST_DIR_IN=.test_in
    TEST_DIR_OUT=.test_out

    rm -fr .cb
    rm -fr $TEST_DIR_IN
    rm -fr $TEST_DIR_OUT

    mkdir -p $TEST_DIR_IN
    mkdir -p $TEST_DIR_OUT

    function generate_randcontent() {
        for i in {1..10000}; do
            echo -e "$i \t $RANDOM\n"
        done
    }

    RAND_PATH=""
    function generate_randpath() {
        RAND_PATH=$1
        # mkdir
        if [[ $RANDOM -gt 15000 ]];
        then
            if [ -f "$RAND_PATH" ]; then
                RAND_PATH=$1_$RANDOM
                generate_randpath $RAND_PATH
            else
                mkdir -p $RAND_PATH
                generate_randpath $RAND_PATH/$RANDOM
            fi
        else
            if [ -d "$RAND_PATH" ]; then
                RAND_PATH=$1/$RANDOM
                generate_randpath $RAND_PATH
            fi
        fi
    }

    function generate_randfile() {
        for i in {1..1000}; do
            generate_randpath $TEST_DIR_IN
            rand_text=$(generate_randcontent)
            echo $rand_text >> $RAND_PATH
        done
    }

    echo -e "\n\n\n"
    echo "generate $FILE_LENGTH randfile"
    time generate_randfile

    echo -e "\n\n\n"
    echo "./cbsp -c .cb $TEST_DIR_IN"
    time ./cbsp -c .cb $TEST_DIR_IN

    echo -e "\n\n\n"
    echo "./cbsp -s .cb $TEST_DIR_OUT"
    time ./cbsp -s .cb $TEST_DIR_OUT

    DIFF=$(diff -qr $TEST_DIR_IN $TEST_DIR_OUT)
    if [[ $DIFF != "" ]]; then
        echo $DIFF
        echo "!!!!!!!!!!!!!!!!!!!FAILED!!!!!!!!!!!!!!!!!!!"
    else
        rm -fr $TEST_DIR_IN
        rm -fr $TEST_DIR_OUT
        rm -fr .cb
        echo "!!!!!!!!!!!!!!!!!!!SUCCESS!!!!!!!!!!!!!!!!!!!"
    fi
}

test

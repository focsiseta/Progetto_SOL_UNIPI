valgrind --leak-check=full --fair-sched=yes --track-origins=yes ./server config.txt &
sleep 3
SERVER_PID=$!




./client -W ./test_folder/test -f /tmp/sok -t 200 -w ./test_folder/,3 -d ./whereToWrite/ -t 200 -r ./config.txt -c ./config.txt -W ./config.txt -R 3 -d ./whereToWrite/ -p &
#sleep 1




sleep 2
./sighup $SERVER_PID

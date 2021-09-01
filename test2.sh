./server config2.txt &
SERVER_PID=$!
sleep 2


./client -W ./test_folder/test -f /tmp/sok -t 200 -w ./test_folder/,10 -d ./whereToWrite/ -t 200 -r ./config.txt -p -c ./config.txt -W ./config.txt -R 0 -d ./whereToWrite -p &
sleep 1
./client -f /tmp/sok -t 200 -d ./whereToWrite/ -t 200 -w ./test_folder/,20 -p -c ./config.txt -W ./config.txt -R 4 -d ./whereToWrite/ -p &



sleep 2

./sighup $SERVER_PID

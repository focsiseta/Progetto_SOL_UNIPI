Progetto realizzato per l'esame di sistemi operativi


Biggest mistake of all:

My server spawns a thread per connection, if there is a malicious user he can spawn 1000 clients on his machine and send 1000 requests in parallel.
If the machine is powerful enough this shouldn't be a problem, but beware of this problem if you are trying to run this on a toaster.
I need to work on a load balancer, I was thinking about multiple selects on a small number of threads that handle all connetions,but I need a workaround for select waking up in parallel in all the threads.

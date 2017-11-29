extern crate chan;
use std::thread;

fn main() {
    // TODO: parse command line arguments

    // constants
    // TODO: give them types
    let producer_count = 5;
    let consumer_count = 5;

    let buffer_size: usize = 100;
    let elements_per_producer = 100;

    let r = {
        let (s, r) = chan::sync(buffer_size);
        for id in 0..producer_count {
            let s = s.clone();

            // Since we don't save the thread handle returned by thread::spawn(), it is
            // "dropped," so this thread will become "detached." It automatically dies
            // when it has nothing left to do.
            thread::spawn(move || {
                for elem in 0..elements_per_producer {
                    s.send(elem);
                    println!("Producer id {} added {} to buffer.", id, elem);
                }
            });
        }

        /* This extra scoping will drop the initial sender s we created.
         * The channel closes when all the senders are dropped. Assuming
         * the threads outlive this scope, the channel will close once all
         * threads spawned above complete. */
        r
    };

    // A wait group lets us synchronize the completion of multiple threads.
    let wg = chan::WaitGroup::new();
    for id in 0..consumer_count {
        wg.add(1);
        let wg = wg.clone();
        let r = r.clone();
        thread::spawn(move || {
            for elem in r {
                println!("Consumer id {} read {} from buffer.", id, elem)
            }
            wg.done();
        });
    }

    // If this was the end of the process and we didn't call `wg.wait()`, then
    // the process might quit before all of the consumers were done.
    // `wg.wait()` will block until all `wg.done()` calls have finished.
    wg.wait();
}


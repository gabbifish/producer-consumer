// for Rust channels
extern crate chan;
use chan::{Receiver, Sender, WaitGroup};
use std::{thread, time};

// for argument parsing
extern crate getopts;
use getopts::{Matches, Options};
use std::env;

fn producer_basic(id: u32, elems_per_producer: u32, s: Sender<u32>, sleep: bool) {
  for elem in 0..elems_per_producer {
    if sleep {
      // simulate a blocking disk I/O read, sleep for 1ms
      thread::sleep(time::Duration::from_millis(1));
    }
    s.send(elem);
    // println!("Producer id {} added {} to buffer.", id, elem);
  }
}

fn consumer_basic(id: u32, r: Receiver<u32>, wg: WaitGroup, sleep: bool) {
  for elem in r {
    // println!("Consumer id {} read {} from buffer.", id, elem);
    if sleep {
      // simulate an expensive computation, sleep for 1ms
      thread::sleep(time::Duration::from_millis(1));
    }

    let mut val = elem;
    val *= 2; // "data computation"
  }
  wg.done();
}

fn parse_num<T>(matches: &Matches, flag: &str, default: T) -> T 
  where T: std::str::FromStr
{
  match matches.opt_str(flag) {
    Some(x) => match x.parse() {
      Ok(n) => n,
      Err(_) => {
        println!("Error: argument for {} had invalid type", flag);
        ::std::process::exit(1);
      }
    },
    None => default
  }
}

fn print_usage(program: &str, opts: Options) {
  let brief = format!("Usage: {} [-b BUFFER_SIZE] [-p NUM_PRODUCERS] [-c NUM_CONSUMERS] [-e ELEMS_PER_PRODUCER] [-i] [-j]", program);
  print!("{}", opts.usage(&brief));
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let program = &args[0];

    let mut opts = Options::new();
    opts.optflag("h", "help", "print this help menu");
    opts.optopt("b", "", "buffer size in number of integers, default=100", "BUFFER_SIZE");
    opts.optopt("p", "", "number of producer threads, default=5", "NUM_PRODUCERS");
    opts.optopt("c", "", "number of consumer threads, default=5", "NUM_CONSUMERS");
    opts.optopt("e", "", "number of elements each producer adds to buffer, default=100", "ELEMS_PER_PRODUCER");
    opts.optflag("i", "", "have producer threads sleep for 1ms before adding int to buffer");
    opts.optflag("j", "", "have consumer threads sleep for 1ms after reading int from buffer");
    let matches: Matches = match opts.parse(&args[1..]) {
      Ok(m) => { m }
      Err(f) => { panic!(f.to_string()) }
    };
    if matches.opt_present("h") {
      print_usage(program, opts);
      return;
    }

    // default parameters
    let producer_count = parse_num(&matches, "p", 5);
    let consumer_count = parse_num(&matches, "c", 5);
    let buffer_size: usize = parse_num(&matches, "b", 100);
    let elems_per_producer = parse_num(&matches, "e", 100);
    let producer_sleep = matches.opt_present("i");
    let consumer_sleep = matches.opt_present("j");

    // println!("Parameters: p={}, c={}, b={}, e={}, i={}, j={}",
    //     producer_count, consumer_count, buffer_size, elems_per_producer, producer_sleep, consumer_sleep);
    
    let r = {
        let (s, r) = chan::sync(buffer_size);
        for id in 0..producer_count {
            let s = s.clone();

            // Since we don't save the thread handle returned by thread::spawn(), it is
            // "dropped," so this thread will become "detached." It automatically dies
            // when it has nothing left to do.
            thread::spawn(move ||
              producer_basic(id, elems_per_producer, s, producer_sleep)
            );
        }

        /* This extra scoping will drop the initial sender s we created.
         * The channel closes when all the senders are dropped. Assuming
         * the threads outlive this scope, the channel will close once all
         * threads spawned above complete. */
        r
    };

    // A wait group lets us synchronize the completion of multiple threads.
    let wg: WaitGroup = WaitGroup::new();
    for id in 0..consumer_count {
        wg.add(1);
        let wg = wg.clone();
        let r = r.clone();
        thread::spawn(move ||
          consumer_basic(id, r, wg, consumer_sleep)
        );
    }

    // If this was the end of the process and we didn't call `wg.wait()`, then
    // the process might quit before all of the consumers were done.
    // `wg.wait()` will block until all `wg.done()` calls have finished.
    wg.wait();
}

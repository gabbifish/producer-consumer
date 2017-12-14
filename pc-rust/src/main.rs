// for Rust channels
extern crate chan;
use chan::{Receiver, Sender, WaitGroup};
use std::thread;

// for argument parsing
extern crate getopts;
use getopts::{Matches, Options};
use std::env;

fn producer_basic(id: u32, elems_per_producer: u32, s: Sender<u32>) {
  for elem in 0..elems_per_producer {
    s.send(elem);
    println!("Producer id {} added {} to buffer.", id, elem);
  }
}

fn consumer_basic(id: u32, r: Receiver<u32>, wg: WaitGroup) {
  for elem in r {
    println!("Consumer id {} read {} from buffer.", id, elem);
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
  let brief = format!("Usage: {} -b BUFFER_SIZE -p NUM_PRODUCERS -c NUM_CONSUMERS -e ELEMS_PER_PRODUCER", program);
  print!("{}", opts.usage(&brief));
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let program = &args[0];

    let mut opts = Options::new();
    opts.optflag("h", "help", "print this help menu");
    opts.optopt("b", "", "buffer size, in bytes", "BUFFER_SIZE");
    opts.optopt("p", "", "number of producer threads", "NUM_PRODUCERS");
    opts.optopt("c", "", "number of consumer threads", "NUM_CONSUMERS");
    opts.optopt("e", "", "number of elements for each producer to produce", "ELEMS_PER_PRODUCER");
    let matches: Matches = match opts.parse(&args[1..]) {
      Ok(m) => { m }
      Err(f) => { panic!(f.to_string()) }
    };
    if matches.opt_present("h") {
      print_usage(program, opts);
      return;
    }

    // default parameters
    let producer_count = parse_num(&matches, "p", 25);
    let consumer_count = parse_num(&matches, "c", 5);
    let buffer_size: usize = parse_num(&matches, "b", 100);
    let elems_per_producer = parse_num(&matches, "e", 10000);

    println!("Parameters: p={}, c={}, b={}, e={}", producer_count, consumer_count, buffer_size, elems_per_producer);
    
    let r = {
        let (s, r) = chan::sync(buffer_size);
        for id in 0..producer_count {
            let s = s.clone();

            // Since we don't save the thread handle returned by thread::spawn(), it is
            // "dropped," so this thread will become "detached." It automatically dies
            // when it has nothing left to do.
            thread::spawn(move || producer_basic(id, elems_per_producer, s));
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
        thread::spawn(move || consumer_basic(id, r, wg));
    }

    // If this was the end of the process and we didn't call `wg.wait()`, then
    // the process might quit before all of the consumers were done.
    // `wg.wait()` will block until all `wg.done()` calls have finished.
    wg.wait();
}


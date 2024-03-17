两层filter，一层rolling sketch

```c
header meta_h {
    bit<16> filter_index_1;			//filter 1 index
    bit<16> filter_index_2;			//filter 2 index
    bit<16> bucket_index;			//rolling sketch index
    bit<16> freq;					//interval frequence record
    bit<16> max_freq;				
    bit<16> min_freq;				
    bit<8> old_ts_1;				
    bit<8> old_ts_2;				
    bit<8> cur_ts;					
    bit<8> filter_state_1;			//state: {0,cur_ts - old_ts_1 == 0}, {1, cur_ts - old_ts_1 == 1}, {2, cur_ts - old_ts_1 > 1}
    bit<8> filter_state_2;			//same as above
    bit<8> filter_counter_add_1;	//state == 0, add 0; state == 1, add 1; state == 2, counter clear
    bit<8> filter_counter_add_2;
    bit<8> cur_filter_counter_1;	//read from filter counter
    bit<8> cur_filter_counter_2;
    bit<8> filter_pass_state;		//state: {0, add freq}, {1, update freq}, {2, clear freq}
}
```



logic:

```c
//filter ts
if(cur_ts - old_ts == 0) {
    filter_state = 0;
}
else if(cur_ts - old_ts == 1) {
    filter_state = 1;
}
else {
    filter_state = 2;
}

//filter counter
if(filter_state == 0) {
    counter = old_counter;
}
else if(filter_state == 1) {
    counter = old_counter+1;
}
else {
    counter = 1;		
}
cur_filter_counter = old_counter;

//rolling sketch
filter_pass_state = max(filter_state_1, filter_state_2);
if(filter_pass_state == 0) {
    freq += 1;
}
else(filter_pass_state >= 1) {
    freq = 1;
}
out_data = old_freq;

if(filter_pass_state == 0) {
    read_max_freq;
    read_min_freq;
}
else if(filter_pass_state == 1) {
    max_freq = max(max_freq, freq);
    min_freq = min(min_freq, freq);
}
else if(filter_pass_state == 2) {
    max_freq = 0;
    min_freq = 0xffff;
}

//judge
if(filter_counter >= filter_threshold&&max_freq-min_freq<= delta_threshold) {
    pass;
}
else {
    drop;
}
```


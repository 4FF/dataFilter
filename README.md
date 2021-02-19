# dataFilter
Filter for gibberish in market data

//Initialy for filtering out anomalies in daily yahoo data. Using database of 285k instruments.

//Data files probably were filtered on load for 20-50kb+ files(to conserve ram). 

//filtering = considering data point or period useless.

//Dev process was making 1 filtering function by 1 until no visually strange and too predictable edges were found in data. 

//After each wave of most predictable anomalies got filtered out, new anomalies were found and later manually confirmed using random selection and visualization tools.

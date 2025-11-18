# Appendix Generator - LLM Prompts

This quick and dirty tool takes the entire LLM history from my LLM provider (which I have manually dumped to `input.json`) and filters by thread id and message id. This allows me to include only the threads relevant to this FYP without including any of my personal LLM threads. The data is dumped into `appendix_llm_prompts.tex` for inclusion in both the interim and final report.

Proper formatting and escape logic was implemented to get this to work nicely.

Run `node app.js` from this directory and the result will be outputted to `appendix_llm_prompts.tex`.

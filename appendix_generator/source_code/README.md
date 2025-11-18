# Appendix Generator - Source Code

This quick and dirty tool takes all files in the codebase (except some explicitly ignored patterns) and dumps their contents into a latex file for easy inclusion in the interim and final report.

Proper formatting and escape logic was implemented to get this to work nicely.

Run `node app.js` from this directory and the result will be outputted to `appendix_source_code.tex`.

set laststatus=0 sw=2 ts=2 expandtab matchpairs=(:),{:},[:],<:> foldmethod=marker foldmarker=[[[,]]]

let c_no_curly_error = 1
syn on

map <F9> ;silent! make run<CR><C-l>;cc<CR>

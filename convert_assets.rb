#!/usr/bin/env ruby

["p1_won", "p2_won", "p1_played", "p2_played", "pause"].each do |asset|
  system("nin10kit --mode=3 #{asset} assets/#{asset}.png && mv #{asset}.h include/. && mv #{asset}.c src/.")
end
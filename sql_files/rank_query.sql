
select symbol, close_p,cast( percent_rank() over(order by close_p) *100 as int) as rank from new_stock_data.current_data  where date = '2023-01-18' order by rank desc, symbol asc limit 2000 ;

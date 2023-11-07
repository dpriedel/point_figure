CREATE OR REPLACE VIEW new_stock_data.vw_percent_change date, symbol, pct_close_chg
AS select symbol, data, adjclose OVER (partition by symbol, order by date asc):

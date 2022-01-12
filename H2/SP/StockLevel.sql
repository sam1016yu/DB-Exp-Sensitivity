CREATE ALIAS STOCKLEVEL AS $$
import java.sql.*;
import java.util.Random;
@CODE
void stockLevel(Connection conn, int terminalDistrictLowerID, int terminalDistrictUpperID, 
    int numWarehouses, int w_id) throws SQLException
{
    
    String stockGetDistOrderIdSQL = "SELECT D_NEXT_O_ID " +
                    "  FROM district " +
                    " WHERE D_W_ID = ? " +
                    "   AND D_ID = ?";

    String stockGetCountStockSQL = "SELECT COUNT(DISTINCT (S_I_ID)) AS STOCK_COUNT " +
                    " FROM order_line, stock " +
                    " WHERE OL_W_ID = ?" +
                    " AND OL_D_ID = ?" +
                    " AND OL_O_ID < ?" +
                    " AND OL_O_ID >= ?" +
                    " AND S_W_ID = ?" +
                    " AND S_I_ID = OL_I_ID" +
                    " AND S_QUANTITY < ?";


    // Stock Level Txn
    try (PreparedStatement stockGetDistOrderId = conn.prepareStatement(stockGetDistOrderIdSQL);
        PreparedStatement stockGetCountStock = conn.prepareStatement(stockGetCountStockSQL)) {

        Random gen = new Random();
        int threshold = (int) (gen.nextDouble() * (20 - 10 + 1) + 10);
        int d_id = (int) (gen.nextDouble() * (terminalDistrictUpperID - terminalDistrictLowerID + 1) + terminalDistrictLowerID);
        
        int o_id;
        int stock_count;

        stockGetDistOrderId.setInt(1, w_id);
        stockGetDistOrderId.setInt(2, d_id);
        
        try (ResultSet rs = stockGetDistOrderId.executeQuery()) {
            if (!rs.next()) {
                throw new RuntimeException("D_W_ID=" + w_id + " D_ID=" + d_id + " not found!");
            }
            o_id = rs.getInt("D_NEXT_O_ID");
        }

        stockGetCountStock.setInt(1, w_id);
        stockGetCountStock.setInt(2, d_id);
        stockGetCountStock.setInt(3, o_id);
        stockGetCountStock.setInt(4, o_id - 20);
        stockGetCountStock.setInt(5, w_id);
        stockGetCountStock.setInt(6, threshold);
        
        try (ResultSet rs = stockGetCountStock.executeQuery()) {
            if (!rs.next()) {
                String msg = String.format("Failed to get StockLevel result for COUNT query " +
                            "[W_ID=%d, D_ID=%d, O_ID=%d]", w_id, d_id, o_id);
                throw new RuntimeException(msg);
            }
            stock_count = rs.getInt("STOCK_COUNT");           
        }
    }

}
$$;
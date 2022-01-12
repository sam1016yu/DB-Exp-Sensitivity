CREATE ALIAS DELIVERY AS $$
import java.math.BigDecimal;
import java.sql.*;
import java.util.Random;
@CODE
void delivery(Connection conn, int terminalDistrictLowerID, int terminalDistrictUpperID, 
    int numWarehouses, int w_id) throws SQLException
{

    String delivGetOrderIdSQL = "SELECT NO_O_ID FROM new_order " +
                    " WHERE NO_D_ID = ? " +
                    "   AND NO_W_ID = ? " +
                    " ORDER BY NO_O_ID ASC " +
                    " LIMIT 1";

    String delivDeleteNewOrderSQL = "DELETE FROM new_order " +
                    " WHERE NO_O_ID = ? " +
                    "   AND NO_D_ID = ?" +
                    "   AND NO_W_ID = ?";

    String delivGetCustIdSQL = "SELECT O_C_ID FROM oorder " +
                    " WHERE O_ID = ? " +
                    "   AND O_D_ID = ? " +
                    "   AND O_W_ID = ?";

    String delivUpdateCarrierIdSQL = "UPDATE oorder " +
                    "   SET O_CARRIER_ID = ? " +
                    " WHERE O_ID = ? " +
                    "   AND O_D_ID = ?" +
                    "   AND O_W_ID = ?";

    String delivUpdateDeliveryDateSQL = "UPDATE order_line " +
                    "   SET OL_DELIVERY_D = ? " +
                    " WHERE OL_O_ID = ? " +
                    "   AND OL_D_ID = ? " +
                    "   AND OL_W_ID = ? ";

    String delivSumOrderAmountSQL = "SELECT SUM(OL_AMOUNT) AS OL_TOTAL " +
                    "  FROM order_line " +
                    " WHERE OL_O_ID = ? " +
                    "   AND OL_D_ID = ? " +
                    "   AND OL_W_ID = ?";

    String delivUpdateCustBalDelivCntSQL = "UPDATE customer " +
                    "   SET C_BALANCE = C_BALANCE + ?," +
                    "       C_DELIVERY_CNT = C_DELIVERY_CNT + 1 " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ? ";


    Random gen = new Random();
    int o_carrier_id = (int) (gen.nextDouble() * (10 - 1 + 1) + 1);
    Timestamp timestamp = new Timestamp(System.currentTimeMillis());

    // Delivery Txn
    try (PreparedStatement delivGetOrderId = conn.prepareStatement(delivGetOrderIdSQL);
        PreparedStatement delivDeleteNewOrder = conn.prepareStatement(delivDeleteNewOrderSQL);
        PreparedStatement delivGetCustId = conn.prepareStatement(delivGetCustIdSQL);
        PreparedStatement delivUpdateCarrierId = conn.prepareStatement(delivUpdateCarrierIdSQL);
        PreparedStatement delivUpdateDeliveryDate = conn.prepareStatement(delivUpdateDeliveryDateSQL);
        PreparedStatement delivSumOrderAmount = conn.prepareStatement(delivSumOrderAmountSQL);
        PreparedStatement delivUpdateCustBalDelivCnt = conn.prepareStatement(delivUpdateCustBalDelivCntSQL)) {

        int d_id, c_id;
        float ol_total;
        int[] orderIDs;

        orderIDs = new int[10];
        for (d_id = 1; d_id <= terminalDistrictUpperID; d_id++) {
            delivGetOrderId.setInt(1, d_id);
            delivGetOrderId.setInt(2, w_id);

            int no_o_id;
            try (ResultSet rs = delivGetOrderId.executeQuery()) {
                if (!rs.next()) {
                    // This district has no new orders
                    // This can happen but should be rare
                    continue;
                }

                no_o_id = rs.getInt("NO_O_ID");
                orderIDs[d_id - 1] = no_o_id;
            }

            delivDeleteNewOrder.setInt(1, no_o_id);
            delivDeleteNewOrder.setInt(2, d_id);
            delivDeleteNewOrder.setInt(3, w_id);
            
            int result = delivDeleteNewOrder.executeUpdate();
                
            if (result != 1) {
                    // This code used to run in a loop in an attempt to make this work
                    // with MySQL default weird consistency level. We just always run
                    // this as SERIALIZABLE instead. I dont think that fixing this one
                    // error makes this work with MySQL default consistency.
                    // Careful auditing would be required.
                String msg = String.format("NewOrder delete failed. Not running with SERIALIZABLE isolation? " +
                            "[w_id=%d, d_id=%d, no_o_id=%d]", w_id, d_id, no_o_id);
                throw new RuntimeException(msg);
            }


            delivGetCustId.setInt(1, no_o_id);
            delivGetCustId.setInt(2, d_id);
            delivGetCustId.setInt(3, w_id);
                
            try (ResultSet rs = delivGetCustId.executeQuery()) {
                if (!rs.next()) {
                    String msg = String.format("Failed to retrieve ORDER record [W_ID=%d, D_ID=%d, O_ID=%d]",
                                w_id, d_id, no_o_id);
                    throw new RuntimeException(msg);
                }
                c_id = rs.getInt("O_C_ID");
            }

            delivUpdateCarrierId.setInt(1, o_carrier_id);
            delivUpdateCarrierId.setInt(2, no_o_id);
            delivUpdateCarrierId.setInt(3, d_id);
            delivUpdateCarrierId.setInt(4, w_id);
                
            result = delivUpdateCarrierId.executeUpdate();
                
            if (result != 1) {
                String msg = String.format("Failed to update ORDER record [W_ID=%d, D_ID=%d, O_ID=%d]",
                            w_id, d_id, no_o_id);
                throw new RuntimeException(msg);
            }

            delivUpdateDeliveryDate.setTimestamp(1, timestamp);
            delivUpdateDeliveryDate.setInt(2, no_o_id);
            delivUpdateDeliveryDate.setInt(3, d_id);
            delivUpdateDeliveryDate.setInt(4, w_id);
            
            result = delivUpdateDeliveryDate.executeUpdate();
            
            if (result == 0) {
                String msg = String.format("Failed to update ORDER_LINE records [W_ID=%d, D_ID=%d, O_ID=%d]",
                            w_id, d_id, no_o_id);
                throw new RuntimeException(msg);
            }


            delivSumOrderAmount.setInt(1, no_o_id);
            delivSumOrderAmount.setInt(2, d_id);
            delivSumOrderAmount.setInt(3, w_id);
            
            try (ResultSet rs = delivSumOrderAmount.executeQuery()) {
                if (!rs.next()) {
                    String msg = String.format("Failed to retrieve ORDER_LINE records [W_ID=%d, D_ID=%d, O_ID=%d]",
                                w_id, d_id, no_o_id);
                    throw new RuntimeException(msg);
                }
                ol_total = rs.getFloat("OL_TOTAL");
            }

            int idx = 1; // HACK: So that we can debug this query
            delivUpdateCustBalDelivCnt.setDouble(idx++, ol_total);
            delivUpdateCustBalDelivCnt.setInt(idx++, w_id);
            delivUpdateCustBalDelivCnt.setInt(idx++, d_id);
            delivUpdateCustBalDelivCnt.setInt(idx, c_id);
            
            result = delivUpdateCustBalDelivCnt.executeUpdate();
            
            if (result == 0) {
                String msg = String.format("Failed to update CUSTOMER record [W_ID=%d, D_ID=%d, C_ID=%d]",
                            w_id, d_id, c_id);
                throw new RuntimeException(msg);
            }
        }
    }

}
$$;
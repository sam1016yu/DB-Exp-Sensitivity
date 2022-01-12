CREATE ALIAS PAYMENT AS $$
import java.sql.*;
import java.util.Random;
import java.util.ArrayList;
@CODE
void payment(Connection conn, int terminalDistrictLowerID, int terminalDistrictUpperID, 
    int numWarehouses, int w_id) throws SQLException
{

    String payUpdateWhseSQL = "UPDATE warehouse SET W_YTD = W_YTD + ? WHERE W_ID = ?";

    String payGetWhseSQL = "SELECT W_STREET_1, W_STREET_2, W_CITY, W_STATE, W_ZIP, W_NAME FROM warehouse WHERE W_ID = ?";

    String payUpdateDistSQL = "UPDATE district SET D_YTD = D_YTD + ? WHERE D_W_ID = ? AND D_ID = ?";

    String payGetDistSQL = "SELECT D_STREET_1, D_STREET_2, D_CITY, D_STATE, D_ZIP, D_NAME FROM district WHERE D_W_ID = ? AND D_ID = ?";

    String payGetCustSQL = "SELECT C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, " +
                    "       C_CITY, C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, " +
                    "       C_DISCOUNT, C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT, C_SINCE " +
                    "  FROM customer " +
                    "  WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String payGetCustCdataSQL = "SELECT C_DATA " +
                    "  FROM customer " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String payUpdateCustBalCdataSQL = "UPDATE customer " +
                    "   SET C_BALANCE = ?, " +
                    "       C_YTD_PAYMENT = ?, " +
                    "       C_PAYMENT_CNT = ?, " +
                    "       C_DATA = ? " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String payUpdateCustBalSQL = "UPDATE customer " +
                    "   SET C_BALANCE = ?, " +
                    "       C_YTD_PAYMENT = ?, " +
                    "       C_PAYMENT_CNT = ? " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_ID = ?";

    String payInsertHistSQL = "INSERT INTO history " +
                    " (H_C_D_ID, H_C_W_ID, H_C_ID, H_D_ID, H_W_ID, H_DATE, H_AMOUNT, H_DATA) " +
                    " VALUES (?,?,?,?,?,?,?,?)";

    String customerByNameSQL = "SELECT C_FIRST, C_MIDDLE, C_ID, C_STREET_1, C_STREET_2, C_CITY, " +
                    "       C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, " +
                    "       C_BALANCE, C_YTD_PAYMENT, C_PAYMENT_CNT, C_SINCE " +
                    "  FROM customer " +
                    " WHERE C_W_ID = ? " +
                    "   AND C_D_ID = ? " +
                    "   AND C_LAST = ? " +
                    " ORDER BY C_FIRST";

    String[] nameTokens = {"BAR", "OUGHT", "ABLE", "PRI", "PRES", "ESE", "ANTI", "CALLY", "ATION", "EING"};

    try (PreparedStatement payUpdateWhse = conn.prepareStatement(payUpdateWhseSQL);
        PreparedStatement payGetWhse = conn.prepareStatement(payGetWhseSQL);
        PreparedStatement payUpdateDist = conn.prepareStatement(payUpdateDistSQL);
        PreparedStatement payGetDist = conn.prepareStatement(payGetDistSQL);
        PreparedStatement payGetCustCdata = conn.prepareStatement(payGetCustCdataSQL);
        PreparedStatement payUpdateCustBalCdata = conn.prepareStatement(payUpdateCustBalCdataSQL);
        PreparedStatement payUpdateCustBal = conn.prepareStatement(payUpdateCustBalSQL);
        PreparedStatement payInsertHist = conn.prepareStatement(payInsertHistSQL)) {

        Random gen = new Random();
        int c_id;
        int c_payment_cnt;
        int c_delivery_cnt;
        Timestamp c_since;
        float c_discount;
        float c_credit_lim;
        float c_balance;
        float c_ytd_payment;
        String c_credit; 
        String c_last;
        String c_first;
        String c_street_1;
        String c_street_2;
        String c_city;
        String c_state;
        String c_zip;
        String c_phone;
        String c_middle;
        String c_data = null;

        int districtID = (int) (gen.nextDouble() * (terminalDistrictUpperID - terminalDistrictLowerID + 1) + terminalDistrictLowerID);
        int customerID = ((((int) (gen.nextDouble() * (1023 - 0 + 1) + 0) | (int) (gen.nextDouble() * (3000 - 1 + 1) + 1)) + 259) % (3000 - 1 + 1)) + 1;
        int x = (int) (gen.nextDouble() * (100 - 1 + 1) + 1);

        int customerDistrictID;
        int customerWarehouseID;
        if (x <= 85) {
            customerDistrictID = districtID;
            customerWarehouseID = w_id;
        } else {
             customerDistrictID = (int) (gen.nextDouble() * (10 - 1 + 1) + 1);
             do {
                     customerWarehouseID = (int) (gen.nextDouble() * (numWarehouses - 1 + 1) + 1);
             }
             while (customerWarehouseID == w_id && numWarehouses > 1);
        }

        int y = (int) (gen.nextDouble() * (100 - 1 + 1) + 1);
        boolean customerByName;
        String customerLastName = null;
        customerID = -1;

        if (y <= 60) {
            // 60% lookups by last name
            customerByName = true;
            int num = ((((int) (gen.nextDouble() * (255 - 0 + 1) + 0) | (int) (gen.nextDouble() * (999 - 0 + 1) + 0)) + 223) % (999 - 0 + 1)) + 0;
            customerLastName = nameTokens[num / 100] + nameTokens[(num / 10) % 10]
                + nameTokens[num % 10];
        } else {
            // 40% lookups by customer ID
            customerByName = false;
            customerID = ((((int) (gen.nextDouble() * (1023 - 0 + 1) + 0) | (int) (gen.nextDouble() * (3000 - 1 + 1) + 1)) + 259) % (3000 - 1 + 1)) + 1;
        }

        float paymentAmount = (float) (((int) (gen.nextDouble() * (500000 - 100 + 1) + 100)) / 100.0);

        String w_street_1, w_street_2, w_city, w_state, w_zip, w_name;
        String d_street_1, d_street_2, d_city, d_state, d_zip, d_name;

        payUpdateWhse.setDouble(1, paymentAmount);
        payUpdateWhse.setInt(2, w_id);
        
        int result = payUpdateWhse.executeUpdate();
        if (result == 0) {
            throw new RuntimeException("W_ID=" + w_id + " not found!");
        }

        payGetWhse.setInt(1, w_id);
        try (ResultSet rs = payGetWhse.executeQuery()) {
            if (!rs.next()) {
                    throw new RuntimeException("W_ID=" + w_id + " not found!");
            }
            w_street_1 = rs.getString("W_STREET_1");
            w_street_2 = rs.getString("W_STREET_2");
            w_city = rs.getString("W_CITY");
            w_state = rs.getString("W_STATE");
            w_zip = rs.getString("W_ZIP");
            w_name = rs.getString("W_NAME");
        }

        payUpdateDist.setDouble(1, paymentAmount);
        payUpdateDist.setInt(2, w_id);
        payUpdateDist.setInt(3, districtID);
        result = payUpdateDist.executeUpdate();
        if (result == 0) {
            throw new RuntimeException("D_ID=" + districtID + " D_W_ID=" + w_id + " not found!");
        }

        payGetDist.setInt(1, w_id);
        payGetDist.setInt(2, districtID);
        try (ResultSet rs = payGetDist.executeQuery()) {
            if (!rs.next()) {
                    throw new RuntimeException("D_ID=" + districtID + " D_W_ID=" + w_id + " not found!");
            }
            d_street_1 = rs.getString("D_STREET_1");
            d_street_2 = rs.getString("D_STREET_2");
            d_city = rs.getString("D_CITY");
            d_state = rs.getString("D_STATE");
            d_zip = rs.getString("D_ZIP");
            d_name = rs.getString("D_NAME");
        }

        if (customerByName) {
            ArrayList<String> c_firsts = new ArrayList<>();
            ArrayList<String> c_middles = new ArrayList<>();
            ArrayList<String> c_street_1s = new ArrayList<>();
            ArrayList<String> c_street_2s = new ArrayList<>();
            ArrayList<String> c_citys = new ArrayList<>();
            ArrayList<String> c_states = new ArrayList<>();
            ArrayList<String> c_zips = new ArrayList<>();
            ArrayList<String> c_phones = new ArrayList<>();
            ArrayList<String> c_credits = new ArrayList<>();
            ArrayList<Float> c_credit_lims = new ArrayList<>();
            ArrayList<Float> c_discounts = new ArrayList<>();
            ArrayList<Float> c_balances = new ArrayList<>();
            ArrayList<Float> c_ytd_payments = new ArrayList<>();
            ArrayList<Integer> c_payment_cnts = new ArrayList<>();
            ArrayList<Timestamp> c_sinces = new ArrayList<>();
            ArrayList<Integer> c_ids = new ArrayList<>();

            try (PreparedStatement customerByNameS = conn.prepareStatement(customerByNameSQL)) {

                customerByNameS.setInt(1, customerWarehouseID);
                customerByNameS.setInt(2, customerDistrictID);
                customerByNameS.setString(3, customerLastName);
                try (ResultSet rs = customerByNameS.executeQuery()) {

                    while (rs.next()) {
                        c_firsts.add(rs.getString("c_first"));
                        c_middles.add(rs.getString("c_middle"));
                        c_street_1s.add(rs.getString("c_street_1"));
                        c_street_2s.add(rs.getString("c_street_2"));
                        c_citys.add(rs.getString("c_city"));
                        c_states.add(rs.getString("c_state"));
                        c_zips.add(rs.getString("c_zip"));
                        c_phones.add(rs.getString("c_phone"));
                        c_credits.add(rs.getString("c_credit"));
                        c_credit_lims.add(rs.getFloat("c_credit_lim"));
                        c_discounts.add(rs.getFloat("c_discount"));
                        c_balances.add(rs.getFloat("c_balance"));
                        c_ytd_payments.add(rs.getFloat("c_ytd_payment"));
                        c_payment_cnts.add(rs.getInt("c_payment_cnt"));
                        c_sinces.add(rs.getTimestamp("c_since"));
                        c_ids.add(rs.getInt("C_ID"));
                        //c_last = customerLastName;
                    }
                }
            }

            if (c_ids.size() == 0) {
                throw new RuntimeException("C_LAST=" + customerLastName + " C_D_ID=" + customerDistrictID + " C_W_ID=" + customerWarehouseID + " not found!");
            }

            int index = c_ids.size() / 2;
            if (c_ids.size() % 2 == 0) {
                index -= 1;
            }
            c_first = c_firsts.get(index);
            c_middle = c_middles.get(index);
            c_street_1 = c_street_1s.get(index);
            c_street_2 = c_street_2s.get(index);
            c_city = c_citys.get(index);
            c_state = c_states.get(index);
            c_zip = c_zips.get(index);
            c_phone = c_phones.get(index);
            c_credit = c_credits.get(index);
            c_credit_lim = c_credit_lims.get(index);
            c_discount = c_discounts.get(index);
            c_balance = c_balances.get(index);
            c_ytd_payment = c_ytd_payments.get(index);
            c_payment_cnt = c_payment_cnts.get(index);
            c_since = c_sinces.get(index);
            c_id = c_ids.get(index);
            c_last = customerLastName;
        } else {
            try (PreparedStatement payGetCust = conn.prepareStatement(payGetCustSQL)) {

                payGetCust.setInt(1, customerWarehouseID);
                payGetCust.setInt(2, customerDistrictID);
                payGetCust.setInt(3, customerID);

                try (ResultSet rs = payGetCust.executeQuery()) {
                    if (!rs.next()) {
                        throw new RuntimeException("C_ID=" + customerID + " C_D_ID=" + customerDistrictID + " C_W_ID=" + customerWarehouseID + " not found!");
                    }
                    c_first = rs.getString("c_first");
                    c_middle = rs.getString("c_middle");
                    c_street_1 = rs.getString("c_street_1");
                    c_street_2 = rs.getString("c_street_2");
                    c_city = rs.getString("c_city");
                    c_state = rs.getString("c_state");
                    c_zip = rs.getString("c_zip");
                    c_phone = rs.getString("c_phone");
                    c_credit = rs.getString("c_credit");
                    c_credit_lim = rs.getFloat("c_credit_lim");
                    c_discount = rs.getFloat("c_discount");
                    c_balance = rs.getFloat("c_balance");
                    c_ytd_payment = rs.getFloat("c_ytd_payment");
                    c_payment_cnt = rs.getInt("c_payment_cnt");
                    c_since = rs.getTimestamp("c_since");
                    c_id = customerID;
                    c_last = rs.getString("C_LAST");
                }
            }
        }

        c_balance -= paymentAmount;
        c_ytd_payment += paymentAmount;
        c_payment_cnt += 1;

        if (c_credit.equals("BC")) {
            payGetCustCdata.setInt(1, customerWarehouseID);
            payGetCustCdata.setInt(2, customerDistrictID);
            payGetCustCdata.setInt(3, c_id);
            try (ResultSet rs = payGetCustCdata.executeQuery()) {
                if (!rs.next()) {
                    throw new RuntimeException("C_ID=" + c_id + " C_W_ID=" + customerWarehouseID + " C_D_ID=" + customerDistrictID + " not found!");
                }
                c_data = rs.getString("C_DATA");
            }

            c_data = c_id + " " + customerDistrictID + " " + customerWarehouseID + " " + districtID + " " + w_id + " " + paymentAmount + " | " + c_data;
            if (c_data.length() > 500) {
                c_data = c_data.substring(0, 500);
            }

            payUpdateCustBalCdata.setDouble(1, c_balance);
            payUpdateCustBalCdata.setDouble(2, c_ytd_payment);
            payUpdateCustBalCdata.setInt(3, c_payment_cnt);
            payUpdateCustBalCdata.setString(4, c_data);
            payUpdateCustBalCdata.setInt(5, customerWarehouseID);
            payUpdateCustBalCdata.setInt(6, customerDistrictID);
            payUpdateCustBalCdata.setInt(7, c_id);
            result = payUpdateCustBalCdata.executeUpdate();

            if (result == 0) {
                throw new RuntimeException("Error in PYMNT Txn updating Customer C_ID=" + c_id + " C_W_ID=" + customerWarehouseID + " C_D_ID=" + customerDistrictID);
            }
        } else {

            payUpdateCustBal.setDouble(1, c_balance);
            payUpdateCustBal.setDouble(2, c_ytd_payment);
            payUpdateCustBal.setInt(3, c_payment_cnt);
            payUpdateCustBal.setInt(4, customerWarehouseID);
            payUpdateCustBal.setInt(5, customerDistrictID);
            payUpdateCustBal.setInt(6, c_id);
            result = payUpdateCustBal.executeUpdate();

            if (result == 0) {
                throw new RuntimeException("C_ID=" + c_id + " C_W_ID=" + customerWarehouseID + " C_D_ID=" + customerDistrictID + " not found!");
            }
        }

        if (w_name.length() > 10) {
            w_name = w_name.substring(0, 10);
        }
        if (d_name.length() > 10) {
            d_name = d_name.substring(0, 10);
        }
        String h_data = w_name + "    " + d_name;

        payInsertHist.setInt(1, customerDistrictID);
        payInsertHist.setInt(2, c ustomerWarehouseID);
        payInsertHist.setInt(3, c_id);
        payInsertHist.setInt(4, districtID);
        payInsertHist.setInt(5, w_id);
        payInsertHist.setTimestamp(6, new Timestamp(System.currentTimeMillis()));
        payInsertHist.setDouble(7, paymentAmount);
        payInsertHist.setString(8, h_data);
        payInsertHist.executeUpdate();

        //conn.commit();
    }

}
$$;
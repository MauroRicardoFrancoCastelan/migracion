public partial class frmMigrar : Form
    {
        public frmMigrar()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            // Mostrar InputBox para obtener el nombre de la base de datos
            string databaseName = PromptForDatabaseName();
            if (string.IsNullOrWhiteSpace(databaseName))
            {
                MessageBox.Show("El nombre de la base de datos no puede estar vacío.");
                return;
            }

            // Configuración de las cadenas de conexión
            string sqlServerConnectionString = "Server=ISAI-PC\\SQLEXPRESS;Database=Proyectoy;Integrated Security=True;";
            string mySqlServerConnectionString = "Server=localhost;User Id=root;Password=12345;";

            string[] tablas = { "datAdmin", "Datos", "datosInsumos", "datPersonal", "Gastos", "proveedores", "Ventas", "reportes_diario" };

            using (SqlConnection sqlConnection = new SqlConnection(sqlServerConnectionString))
            {
                sqlConnection.Open();

                using (MySqlConnection mySqlConnection = new MySqlConnection(mySqlServerConnectionString))
                {
                    mySqlConnection.Open();

                    // Crear la base de datos en MySQL
                    string createDatabaseQuery = $"CREATE DATABASE IF NOT EXISTS {databaseName};";
                    using (MySqlCommand createDbCommand = new MySqlCommand(createDatabaseQuery, mySqlConnection))
                    {
                        createDbCommand.ExecuteNonQuery();
                    }

                    // Usar la nueva base de datos
                    mySqlConnection.ChangeDatabase(databaseName);

                    foreach (string tabla in tablas)
                    {
                        // Obtener la estructura de la tabla desde SQL Server
                        string sqlTableQuery = $"SELECT * FROM {tabla}";
                        DataTable schemaTable;

                        using (SqlCommand sqlCommand = new SqlCommand(sqlTableQuery, sqlConnection))
                        using (SqlDataReader sqlReader = sqlCommand.ExecuteReader(CommandBehavior.SchemaOnly))
                        {
                            schemaTable = sqlReader.GetSchemaTable();
                        }

                        // Verificar si la tabla ya existe en MySQL
                        string checkTableQuery = $"SHOW TABLES LIKE '{tabla}';";
                        using (MySqlCommand checkTableCommand = new MySqlCommand(checkTableQuery, mySqlConnection))
                        {
                            var result = checkTableCommand.ExecuteScalar();

                            if (result == null)
                            {
                                // Crear la tabla en MySQL basada en la estructura obtenida de SQL Server
                                string createTableQuery = GenerateCreateTableQuery(tabla, schemaTable);
                                using (MySqlCommand createTableCommand = new MySqlCommand(createTableQuery, mySqlConnection))
                                {
                                    createTableCommand.ExecuteNonQuery();
                                }
                            }
                        }

                        // Migrar los datos
                        using (SqlCommand sqlCommand = new SqlCommand(sqlTableQuery, sqlConnection))
                        using (SqlDataReader sqlReader = sqlCommand.ExecuteReader())
                        {
                            while (sqlReader.Read())
                            {
                                string insertQuery = GenerateInsertQuery(tabla, schemaTable);

                                using (MySqlCommand mySqlCommand = new MySqlCommand(insertQuery, mySqlConnection))
                                {
                                    foreach (DataRow row in schemaTable.Rows)
                                    {
                                        string columnName = row["ColumnName"].ToString();
                                        mySqlCommand.Parameters.AddWithValue($"@{columnName}", sqlReader[columnName]);
                                    }
                                    mySqlCommand.ExecuteNonQuery();
                                }
                            }
                        }
                    }
                    mySqlConnection.Close();
                }
            }
            MessageBox.Show("Migración completada para todas las tablas.");

        }

        private string GenerateCreateTableQuery(string tableName, DataTable schemaTable)
        {
            string createTableQuery = $"CREATE TABLE {tableName} (";

            foreach (DataRow row in schemaTable.Rows)
            {
                string columnName = row["ColumnName"].ToString();
                string columnType = GetMySqlType(row["DataType"].ToString());
                createTableQuery += $"{columnName} {columnType}, ";
            }

            createTableQuery = createTableQuery.TrimEnd(',', ' ') + ");";
            return createTableQuery;
        }

        private string GetMySqlType(string sqlServerType)
        {
            switch (sqlServerType)
            {
                case "System.Int32":
                    return "INT";
                case "System.String":
                    return "VARCHAR(255)"; // Default length for demonstration
                case "System.DateTime":
                    return "DATETIME";
                case "System.Boolean":
                    return "TINYINT(1)";
                case "System.Decimal":
                    return "DECIMAL(18, 2)";
                case "System.Double":
                    return "DOUBLE";
                case "System.Byte[]":
                    return "BLOB";
                case "System.NVarChar":
                    return "VARCHAR(255)"; // Adjust length as needed
                default:
                    throw new Exception($"Tipo de datos no soportado: {sqlServerType}");
            }
        }

        private string GenerateInsertQuery(string tableName, DataTable schemaTable)
        {
            string columns = string.Join(", ", schemaTable.Rows.Cast<DataRow>().Select(row => row["ColumnName"].ToString()));
            string values = string.Join(", ", schemaTable.Rows.Cast<DataRow>().Select(row => $"@{row["ColumnName"].ToString()}"));
            return $"INSERT INTO {tableName} ({columns}) VALUES ({values})";
        }

        private string PromptForDatabaseName()
        {
            using (InputBoxForm inputBox = new InputBoxForm("Ingrese el nombre de la base de datos:"))
            {
                if (inputBox.ShowDialog() == DialogResult.OK)
                {
                    return inputBox.InputText;
                }
                else
                {
                    return string.Empty;
                }
            }
        }
    }
}
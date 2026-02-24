package org.spider.railway.configuration;

import org.flywaydb.core.Flyway;
import org.springframework.boot.autoconfigure.flyway.FlywayProperties;
import org.springframework.boot.autoconfigure.r2dbc.R2dbcProperties;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Primary;
import org.springframework.data.r2dbc.repository.config.EnableR2dbcRepositories;
import org.springframework.r2dbc.core.ConnectionAccessor;
import org.springframework.r2dbc.core.DatabaseClient;
import org.springframework.transaction.annotation.EnableTransactionManagement;
import reactor.util.annotation.NonNull;

@Configuration
@EnableR2dbcRepositories
@EnableTransactionManagement
@EnableConfigurationProperties({ R2dbcProperties.class, FlywayProperties.class })
public class PostgresConfig {
    @NonNull
    @Bean(initMethod = "migrate")
    public Flyway flyway(@NonNull FlywayProperties flywayProperties, @NonNull R2dbcProperties r2dbcProperties) {
        return Flyway.configure()
                .dataSource(
                        toFlywayUrl(r2dbcProperties.getUrl()),
                        r2dbcProperties.getUsername(),
                        r2dbcProperties.getPassword()
                )
                .locations(flywayProperties.getLocations().toArray(String[]::new))
                .baselineOnMigrate(true)
                .load();
    }

    @Bean
    @Primary
    public ConnectionAccessor getConnectionAccessor(DatabaseClient databaseClient) {
        if (databaseClient == null)
            throw new RuntimeException(null + " is not a ConnectionAccessor.");

        return databaseClient;
    }

    /*
    @Bean
    @NonNull
    public ReactiveTransactionManager transactionManager(ConnectionFactory connectionFactory) {
        return new R2dbcTransactionManager(connectionFactory);
    }
    */

    private static String toFlywayUrl(@NonNull String r2dbcUrl) {
        return r2dbcUrl.replaceFirst("^r2dbc:(pool:)?", "jdbc:");
    }
}

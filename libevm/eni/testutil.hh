#pragma once

#define EXPECT_THROW_MSG(stmt, msg)                          \
    try                                                      \
    {                                                        \
        stmt;                                                \
        FAIL() << "Expected exception not thrown by " #stmt; \
    }                                                        \
    catch (const std::runtime_error& err)                    \
    {                                                        \
        EXPECT_STREQ(err.what(), msg);                       \
    }

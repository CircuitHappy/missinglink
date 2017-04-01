#include "missing_link/file.hpp"
#include "missing_link/gpio.hpp"
#include "gtest/gtest.h"

#include <cstring>
#include <memory>

using namespace MissingLink;
using std::string;

namespace {

class FakeFile : public File {
public:

  class StorageProxy {
  public:
    StorageProxy() : isOpen(false) {
      ::memset(buffer, 0, 128);
    }
    bool isOpen;
    char buffer[128];
  };

  FakeFile(const string strPath, const Access access)
    : File(strPath, access)
    , storageProxy(std::shared_ptr<StorageProxy>(new StorageProxy()))
  {}

  std::shared_ptr<StorageProxy> storageProxy;

private:

  bool isOpen() const override {
    return storageProxy->isOpen;
  }

  int open() override {
    storageProxy->isOpen = true;
    return 0;
  }

  int close() override {
    storageProxy->isOpen = false;
    return 0;
  }

  int write(const char *buf, const size_t nBytes) override {
    ::memcpy(storageProxy->buffer, buf, nBytes);
    return 0;
  }

  int read(char *buf, const size_t nBytes) override {
    ::memcpy(buf, storageProxy->buffer, nBytes);
    return 0;
  }
};

class TestSysfsPin : public GPIO::SysfsPin {
public:
  TestSysfsPin(const int address, const GPIO::Pin::Direction direction)
    : SysfsPin(address, direction)
  {}

  std::map<string, std::shared_ptr<FakeFile::StorageProxy>> touchedFileStorage;

private:
  std::unique_ptr<File> createFile(const string strPath, const File::Access access) override {
    auto file = new FakeFile(strPath, access);
    touchedFileStorage[strPath] = file->storageProxy;
    return std::unique_ptr<File>(file);
  }

};

}

TEST(SysfsPin, ExportsAddressOnExport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  auto fileStorage = pin.touchedFileStorage["/sys/class/gpio/export"];
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "100");
}

TEST(SysfsPin, WritesDirectionOnExport) {
  TestSysfsPin outPin(100, GPIO::Pin::OUT);
  EXPECT_EQ(outPin.Export(), 0);
  auto fileStorage = outPin.touchedFileStorage["/sys/class/gpio/gpio100/direction"];
  ASSERT_NE(fileStorage, nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "out");

  TestSysfsPin inPin(100, GPIO::Pin::IN);
  EXPECT_EQ(inPin.Export(), 0);
  fileStorage = inPin.touchedFileStorage["/sys/class/gpio/gpio100/direction"];
  ASSERT_NE(fileStorage, nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "in");
}

TEST(SysfsPin, OpensValueFileOnExport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  auto fileStorage = pin.touchedFileStorage["/sys/class/gpio/gpio100/value"];
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_TRUE(fileStorage->isOpen);
}

TEST(SysfsPin, UnexportsAddressOnUnexport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.Unexport(), 0);
  auto fileStorage = pin.touchedFileStorage["/sys/class/gpio/unexport"];
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
  EXPECT_STREQ(fileStorage->buffer, "100");
}

TEST(SysfsPin, ClosesValueFileOnUnexport) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);
  EXPECT_EQ(pin.Unexport(), 0);
  auto fileStorage = pin.touchedFileStorage["/sys/class/gpio/gpio100/value"];
  ASSERT_NE(fileStorage.get(), nullptr);
  EXPECT_FALSE(fileStorage->isOpen);
}

TEST(SysfsPin, WritesValues) {
  TestSysfsPin pin(100, GPIO::Pin::OUT);
  EXPECT_EQ(pin.Export(), 0);

  auto fileStorage = pin.touchedFileStorage["/sys/class/gpio/gpio100/value"];
  ASSERT_NE(fileStorage.get(), nullptr);

  EXPECT_EQ(pin.Write(LOW), 0);
  EXPECT_STREQ(fileStorage->buffer, "0");

  EXPECT_EQ(pin.Write(HIGH), 0);
  EXPECT_STREQ(fileStorage->buffer, "1");
}

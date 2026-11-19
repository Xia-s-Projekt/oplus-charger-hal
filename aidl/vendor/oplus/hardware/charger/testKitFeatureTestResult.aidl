// testKitFeatureTestResult.aidl - AIDL parcelable for charger test-kit results.
//
// Field names and types come from the Java view of the same interface:
// RE/java/smali/vendor/oplus/hardware/charger/testKitFeatureTestResult.smali
//   .field public ret:I
//   .field public str:Ljava/lang/String;
package vendor.oplus.hardware.charger;

@VintfStability
parcelable testKitFeatureTestResult {
    int ret;
    String str;
}

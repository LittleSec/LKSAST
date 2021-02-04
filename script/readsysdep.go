package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
)

func main() {
	jsonFile, err := os.Open("weight_ignsame")
	defer jsonFile.Close()
	if err != nil {
		fmt.Println(err)
		return
	}

	byteValue, _ := ioutil.ReadAll(jsonFile)

	var result map[string]map[string]uint32
	json.Unmarshal([]byte(byteValue), &result)

	fmt.Println(result["__se_sys_open"]["__se_sys_close"], result["__se_sys_close"]["__se_sys_open"])
	fmt.Println(result["__se_sys_open"]["__se_sys_write"], result["__se_sys_write"]["__se_sys_open"])
	fmt.Println(result["__se_sys_write"]["__se_sys_close"], result["__se_sys_close"]["__se_sys_write"])
	// file, _ := json.MarshalIndent(result, " ", " ")
	// _ = ioutil.WriteFile("test.json", file, 0644)
}

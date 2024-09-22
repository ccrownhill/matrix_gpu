package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"text/template"

	"gopkg.in/yaml.v3"
)

func main() {
	const usage = "Usage: testgen -o|--outdir <output directory> <test yaml specification file> ..."
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, usage)
		os.Exit(1)
	}

	var outDir string

	flag.StringVar(&outDir, "outdir", "__invalid__", "the output directory")
	flag.StringVar(&outDir, "o", "__invalid__", "the output directory")

	flag.Parse()

	if outDir == "__invalid__" {
		fmt.Fprintln(os.Stderr, usage)
		os.Exit(1)
	}

	if err := os.MkdirAll(outDir, 0775); err != nil {
		fmt.Fprintf(os.Stderr, "Can't create output directory: %v\n", err)
		os.Exit(1)
	}

	baseDir := filepath.Dir(os.Args[0])

	for _, arg := range flag.Args() {
		testGen(arg, baseDir+"/tmpl_tb", baseDir+"/tmpl_script", outDir)
	}
}

func testGen(yamlFile, tmplTb, tmplScript, outDir string) {
	data := map[string]interface{}{}
	buf, err := os.ReadFile(yamlFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Can't read YAML file '%s': %v\n", yamlFile, err)
		os.Exit(1)
	}

	if err := yaml.Unmarshal(buf, &data); err != nil {
		fmt.Fprintf(os.Stderr, "Can't unmarshal YAML file: %v\n", err)
		os.Exit(1)
	}

	yamlDir := filepath.Dir(yamlFile)

	// compile assembler
	asmDir, ok := data["asm_dir"]
	var asmDirStr string
	useAssembly := true
	if ok {
		if str, ok := asmDir.(string); ok {
			if str == "none" {
				useAssembly = false
			} else {

				cmd := exec.Command("make", "bin/assembler")
				asmDirStr = yamlDir + "/" + str
				cmd.Dir = asmDirStr
				if err := cmd.Run(); err != nil {
					fmt.Fprintf(os.Stderr, "Error compiling assembler: %v\n", err)
					os.Exit(1)
				}
			}
		} else {
			fmt.Fprintf(os.Stderr, "Error: assembler path name not a string\n")
			os.Exit(1)
		}

		val, ok := data["asm_basename"]
		if ok && useAssembly {
			if baseName, ok := val.(string); ok {
				fileList, err := filepath.Glob(yamlDir + "/" + baseName + "*.asm")
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error: trying to glob: %v\n", err)
					os.Exit(1)
				}

				cycleCount := 0
				for _, str := range fileList {
					cmd := exec.Command(asmDirStr+"/bin/assembler", "-i", str,
						"-f", "hex")
					out, err := cmd.Output()
					if err != nil {
						fmt.Fprintf(os.Stderr, "Error running assembler: %v\n", err)
						os.Exit(1)
					}

					hexStrVals := strings.Split(string(out), "\n")
					numBlocks := tryParseHexStr(hexStrVals[0])
					numBlocks -= 0xa0000000
					if val, ok := data["is_3d"]; ok {
						if is3d, ok := val.(bool); ok && is3d {

							addInputToData(data, "from_cpu", cycleCount, 0xa0000000+numBlocks+(0b100<<24))
						} else {
							addInputToData(data, "from_cpu", cycleCount, 0xa0000000+numBlocks)
						}
					} else {
						addInputToData(data, "from_cpu", cycleCount, 0xa0000000+numBlocks)
					}

					var instrFlag uint32 = 1
					for i, s := range hexStrVals[1:] {
						if s == "" {
							continue
						}
						intVal := tryParseHexStr(s)

						addInputToData(data, "from_cpu", cycleCount+i+1,
							uint32(intVal)+(instrFlag<<4))
						instrFlag ^= 1
					}

					// the 20 at the end is just an extra safety for waiting
					blocks := 16
					if numBlocks > 16 {
						blocks = int(numBlocks)
					}
					cycleCount += len(hexStrVals) - 1 + blocks*(len(hexStrVals)+15) + 20
				}
			} else {
				fmt.Fprintf(os.Stderr, "Error: assembly file name not a string\n")
				os.Exit(1)
			}
		}

		tb_file := fmt.Sprintf(outDir+"/%s_tb.cpp", data["module"])
		tmplGen(tb_file, tmplTb, data)

		tb_script := fmt.Sprintf(outDir+"/test_%s.sh", data["module"])
		tmplGen(tb_script, tmplScript, data)
	} else {
		fmt.Fprintf(os.Stderr, "Error in YAML: missing 'asm_dir' field\n"+
			"if you don't want to assemble anything set asm_dir to \"none\"\n")
		os.Exit(1)
	}

}

func tmplGen(out_file, tmpl_file string, data map[string]interface{}) {
	funcMap := template.FuncMap{
		"replaceDots": replaceDots,
	}

	out, err := os.Create(out_file)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Can't create '%s': %v\n", out_file, err)
		os.Exit(1)
	}

	tname := filepath.Base(tmpl_file)

	t, err := template.New(tname).Funcs(funcMap).ParseFiles(tmpl_file)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Can't parse template file '%s': %v\n", tmpl_file, err)
		os.Exit(1)
	}

	if err := t.Execute(out, data); err != nil {
		fmt.Fprintf(os.Stderr, "Can't fill template file with values from YAML: %v\n", err)
		os.Exit(1)
	}
}

func tryParseHexStr(hexStr string) uint32 {
	intVal, err := strconv.ParseUint(hexStr, 16, 32)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error: invalid hex number '%s', %v\n", hexStr, err)
		os.Exit(1)
	}
	return uint32(intVal)
}

func addInputToData(data map[string]interface{},
	name string, cycle int, val uint32) {
	m := make(map[string]interface{})
	m["name"] = name
	m["cycle"] = []int{cycle}
	m["val"] = val
	if data["inputs"] == nil {
		data["inputs"] = []interface{}{m}
	} else {
		data["inputs"] =
			[]interface{}(append((data["inputs"]).([]interface{}), m))
	}
}

func replaceDots(mod_name, s interface{}) string {
	mod_str, ok1 := mod_name.(string)
	s_str, ok2 := s.(string)
	if ok1 && ok2 {
		if strings.Contains(s_str, ".") {
			open_bracket_handled := strings.ReplaceAll(s_str, "(", "__BRA__")
			close_bracket_handled := strings.ReplaceAll(
				open_bracket_handled, ")", "__KET__")

			return "rootp->" + mod_str + "__DOT__" +
				strings.ReplaceAll(close_bracket_handled, ".", "__DOT__")
		} else {
			return s_str
		}
	} else {
		fmt.Fprintf(os.Stderr, "Error: expect strings for module name and input\n")
		os.Exit(1)
		return ""
	}
}

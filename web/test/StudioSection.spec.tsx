import { render, screen } from "@testing-library/react";
import userEvent from "@testing-library/user-event";
import { StudioSection } from "../src/App";

const mockSetField = jest.fn();
const mockReset = jest.fn();
const mockRefresh = jest.fn();

jest.mock("../src/hooks/useTps43State", () => {
  const { defaultTps43State } = jest.requireActual<
    typeof import("../src/hooks/tps43Codec")
  >("../src/hooks/tps43Codec");
  return {
    useTps43State: () => ({
      state: defaultTps43State(),
      isConnected: true,
      isLoading: false,
      error: null,
      setField: mockSetField,
      reset: mockReset,
      refresh: mockRefresh,
    }),
  };
});

describe("StudioSection", () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  it("renders all section cards", () => {
    render(<StudioSection />);

    for (const title of [
      "Gestures",
      "Scrolling",
      "Axes",
      "Sensitivity & Filtering",
    ]) {
      expect(screen.getByText(title)).toBeInTheDocument();
    }
  });

  it("prop-drills a Switch toggle to setField with the matching field", async () => {
    const user = userEvent.setup();
    render(<StudioSection />);

    await user.click(screen.getByRole("checkbox", { name: "Flip X" }));

    expect(mockSetField).toHaveBeenCalledWith("flipX", true, true);
  });

  it("wires the Reset and Refresh buttons to the hook", async () => {
    const user = userEvent.setup();
    render(<StudioSection />);

    await user.click(screen.getByRole("button", { name: "Reset to defaults" }));
    await user.click(screen.getByRole("button", { name: "Refresh" }));

    expect(mockReset).toHaveBeenCalledWith(true);
    expect(mockRefresh).toHaveBeenCalled();
  });
});
